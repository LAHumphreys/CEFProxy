#include <CefTestApp.h>
#include <CefTests.h>
#include <gtest/gtest.h>
#include <CefBaseThread.h>
#include "ProxyBrowserApp.h"
#include <CefTestJSBaseTest.h>
#include <stream_client.h>
#include <PipePublisher.h>

#include <SimpleJSON.h>
#include <io_thread.h>
#include <OSTools.h>
#include <WorkerThread.h>

constexpr const char* reqServerURL = "ws://localhost:12345";

const std::string rootPath = OS::Dirname(OS::GetExe());
const std::string initUrl = "file://" + rootPath + "/index.html";

// This is a dirty hack to make the app object available to the tests
CefBaseApp* app;

/**
 * We have to override the application to stop it spinning up a real
 * browser in a GTK window. For our purposes a windowless test browser
 * is fine.
 */
class TestProxyBrowserApp: public ProxyBrowserHandler
{
public:
    TestProxyBrowserApp(CefBaseApp& app)
       : ProxyBrowserHandler(app)
       {}
    void OnContextInitialized() override {
        // Test process will worry about creating a dummy window...
    }

    CefRefPtr<CefBrowser> GetBrowserSync() override {
        return DummyCefApp::GetTestBrowser();
    }
};

int main (int argc, char** argv) {
    CefRefPtr<DummyCefApp> testApp(new DummyCefApp(argc, argv, initUrl));

    app = testApp.get();

    auto browser = std::make_shared<TestProxyBrowserApp>(*testApp);

    std::shared_ptr<ProxyRESTEndPoint> restEndPoint(new ProxyRESTEndPoint(app, browser));

    app->Browser().InstallHandler(browser);

    DummyCefApp::RunTestsAndExit(testApp);
}

/**
 * Subscribe to page loads via the SUB_LOADS API, and publish
 * them out.
 *
 * This simulates our client application - tests should interogate
 * the messages by creating a new PipeSubscriber to receive notifications
 * when a new message is received.
 */
class LoadPublisher: public StreamClient {
public:
    /**
     * C'tor
     *
     * @param url   The uri of the websocket server providing SUB_LOADS
     */
    LoadPublisher(std::string url)
            : StreamClient(std::move(url), "SUB_LOADS", "") {}

    /**
     * Send the subscription request and start publishing responses
     *
     * NOTE: This will block until the subscription is killed, so should
     *       be triggered on a slave thread
     */
    void Start() {
        Run();
    }

    /**
     * The subscription handler is going to return us a JSON string
     * representing the Loaded Page. We will parse it a proper
     * c++ struct before we publish it
     */
    struct LoadedPage {
        static constexpr int FAILED_TO_PARSE = -1;
        int status;
        std::string url;
        std::string source;
    };

    /**
     * Get a new subscription to published loads
     */
    std::shared_ptr<PipeSubscriber<LoadedPage>> NewClient() {
        return publisher.NewClient(1000);
    }

private:
    /**
     * Callback from StreamClient triggered by a new subscription message
     * received
     */
    void OnMessage(const std::string& msg) override {
        // We need a JSON parser to extract the values
        NewStringField(source);
        NewStringField(url);
        NewIntField(status);
        using Response = SimpleParsedJSON<source, url, status>;

        // This method will only ever be called from the subclient thread
        static Response foundParser;
        foundParser.Clear();

        LoadedPage page{-1, "", ""};
        std::string error;
        if (foundParser.Parse(msg.c_str(), error)) {
            page.status = foundParser.Get<status>();
            page.url = foundParser.Get<url>();
            page.status = foundParser.Get<status>();
        } else {
            page.status = LoadedPage::FAILED_TO_PARSE;
            page.url = error;
        }

        publisher.Publish(page);
    }

    PipePublisher<LoadedPage> publisher;
};

class NavigateTest: public JSTestBase {
public:
    NavigateTest() : loadClient(reqServerURL) {}

   /***********************************************************
    *  JSTestBase required API
    ***********************************************************/
    CefBaseApp& App() override {
        return *app;
    }

    CefRefPtr<CefBrowser> TestBrowser() override {
        return DummyCefApp::GetTestBrowser();
    }

    CefRefPtr<CefV8Context> TestContext() override {
        return DummyCefApp::GetTestContext();
    }

    /***********************************************************
     *  Test Utilities
     ***********************************************************/

    /**
     * Trigger a REQ_NAVIGATE request to the proxy server
     *
     * @param requested The url to navigate to
     */
    void RequestNavigate(std::string requested) {
        NewStringField(url);
        SimpleParsedJSON<url> request;
        request.Get<url>() = requested;
        requestThread.Request(reqServerURL, "REQ_NAVIGATE", request.GetJSONString());
    }

    /**
     * Build up a file URL based on the test HTML file
     *
     * @param file The file to get the URL for
     *
     * @return The url
     */
    std::string BuildURL(const std::string& file) {
        return "file://" + rootPath + "/" + file;
    }

    /**
     * Access to the ring-buffer containing the set of loaded pages
     *
     * @return The ring-buffer
     */
    PipeSubscriber<LoadPublisher::LoadedPage>& LoadedPages() {
        return *messageClient;
    }

    /***********************************************************
     *  Test Setup / Tear down
     ***********************************************************/

    void SetUp() override {
        messageClient = loadClient.NewClient();
        clientThread.PostTask([&] () -> void {
            loadClient.Start();
        });
        clientThread.Start();
        loadClient.WaitUntilRunning();

        auto url = BuildURL("dummy.html");
        RequestNavigate(url);
        LoadPublisher::LoadedPage page;

        while (page.url != url) {
            LoadedPages().WaitForMessage(page);
        }
    }

    void TearDown() override {
        LoadPublisher::LoadedPage page;
        JSTestBase::TearDown();
        // Check that we exhausted the published pages...
        ASSERT_FALSE(LoadedPages().GetNextMessage(page)) << page.url;
    }

private:
    IOThread requestThread;

    WorkerThread     clientThread;
    LoadPublisher    loadClient;
    std::shared_ptr<PipeSubscriber<LoadPublisher::LoadedPage>> messageClient;
};

TEST_F(NavigateTest, LoadSimplePage) {
    std::string url = BuildURL("dummy.html");
    RequestNavigate(url);

    LoadPublisher::LoadedPage page;

    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, url);
}

TEST_F(NavigateTest, LoadHtmlRedirect) {
    const std::string redirect_url = BuildURL("redirect.html");
    const std::string index_url = BuildURL("index.html");

    RequestNavigate(redirect_url);

    LoadPublisher::LoadedPage page;

    // Initially we should get the direct page
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, redirect_url);

    // And then the page we're bounced to...
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, index_url);
}

TEST_F(NavigateTest, JSLinkRedirect) {
    const std::string jsUrl = BuildURL("dummy.html");
    const std::string  indexUrl = BuildURL("index.html");

    RequestNavigate(jsUrl);

    LoadPublisher::LoadedPage page;

    // Initially we should get the direct page
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, jsUrl);

    ASSERT_NO_FATAL_FAILURE(
        ExecuteCleanJS(R"JS( window.location.href = "index.html")JS", "index" ".html")
    );

    // Now wait for the redirect to happen...
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, indexUrl);
}

TEST_F(NavigateTest, JSHttpRedirect) {
    const std::string jsUrl = BuildURL("dummy.html");
    const std::string  indexUrl = BuildURL("index.html");

    RequestNavigate(jsUrl);

    LoadPublisher::LoadedPage page;

    // Initially we should get the direct page
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, jsUrl);

    ASSERT_NO_FATAL_FAILURE(
        ExecuteCleanJS(R"JS( window.location.replace("index.html"))JS", "")
    );

    // Now wait for the redirect to happen...
    LoadedPages().WaitForMessage(page);
    ASSERT_EQ(page.url, indexUrl);
}
