#include <CefTestApp.h>
#include <CefTests.h>
#include <gtest/gtest.h>
#include <CefBaseThread.h>
#include "ProxyBrowserApp.h"
#include <CefTestJSBaseTest.h>

#include <SimpleJSON.h>
#include <io_thread.h>
#include <OSTools.h>


class EnableFileCookies: public CefBrowserProcessHandler {
public:
    void OnContextInitialized() {
        // For tests we are using file URLs, and wish to support cookie access...
        CefCookieManager::GetGlobalManager(nullptr)->SetSupportedSchemes({"file"}, false, nullptr);
    }

    IMPLEMENT_REFCOUNTING(EnableFileCookies);
};
CefBaseApp* app;

class TestProxyBrowserApp: public ProxyBrowserHandler,
                           public CefRenderProcessHandler

{
public:
    TestProxyBrowserApp(CefBaseApp& app) : ProxyBrowserHandler(app) {}
    void OnContextInitialized() OVERRIDE {
        // Test process will worry about creating a dummy window...
    }

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override
    {
        // NOTE: Due to strict ordering, the test handler should have already
        //       been invoked - now safe to extract the browser...
        promisedBrowser.set_value(DummyCefApp::GetTestBrowser());
    }

    CefRefPtr<CefBrowser> GetBrowserSync() override {
        return futureBrowser.get();
    }

    std::promise <CefRefPtr<CefBrowser>> promisedBrowser;
    std::future <CefRefPtr<CefBrowser>> futureBrowser;

};

constexpr const char* reqServerURL = "ws://localhost:12345";

const std::string rootPath = OS::Dirname(OS::GetExe());
const std::string initUrl = "file://" + rootPath + "/index.html";

int main (int argc, char** argv) {
    CefRefPtr<DummyCefApp> testApp(new DummyCefApp(argc, argv, initUrl));

    app = testApp.get();

    auto browser = std::make_shared<TestProxyBrowserApp>(*testApp);

    std::shared_ptr<ProxyRESTEndPoint> restEndPoint(new ProxyRESTEndPoint(app, browser));

    app->Browser().InstallHandler(browser);
    app->Browser().InstallHandler(std::make_shared<EnableFileCookies>());
    app->Renderer().InstallHandler(browser);

    DummyCefApp::RunTestsAndExit(testApp);
}

class CookieTest: public JSTestBase {
public:
    CefRefPtr<CefV8Context> TestContext() override {
        return DummyCefApp::GetTestContext();
    }

    CefRefPtr<CefBrowser> TestBrowser() override {
        return DummyCefApp::GetTestBrowser();
    }

    CefBaseApp& App() override {
        return *app;
    }

    void SetUp() override {
        ClearCookies();

        // Wait for the browser to catch up..
        PingBrowser();
    }

    void PingBrowser() {
        std::promise<bool> pong;
        app->IPC().Ping(PID_BROWSER, TestBrowser(), [&] (std::string s) ->
                void {
            pong.set_value(true);
        });
        pong.get_future().wait();
    }

    NewStringField(url);
    using Request = SimpleParsedJSON<url>;
    Request req;

    NewStringField(username);
    using CookieParser = SimpleParsedJSON<username>;
    IOThread requestThread;

};

TEST_F(CookieTest, GetCookie) {
    std::string code = R"JS(
        document.cookie = 'username=Test.User';
        document.cookie
    )JS";
    ExecuteCleanJS(code, "username=Test.User");

    req.Get<url>() = initUrl;
    auto response =
        requestThread.Request(reqServerURL, "REQ_COOKIE_JAR", req.GetJSONString());

    const auto& content = response->WaitForMessage().content_;

    CookieParser parser;
    std::string error;
    ASSERT_TRUE(parser.Parse(content.c_str(), error));
    ASSERT_EQ(parser.Get<username>(), "Test.User");
}

TEST_F(CookieTest, DifferentURL) {
    std::string code = R"JS(
        document.cookie = 'username=Test.User';
        document.cookie
    )JS";
    ExecuteCleanJS(code, "username=Test.User");

    req.Get<url>() = "https://www.google.co.uk";
    auto response =
            requestThread.Request(reqServerURL, "REQ_COOKIE_JAR", req.GetJSONString());

    const auto& content = response->WaitForMessage().content_;

    CookieParser parser;
    std::string error;
    ASSERT_TRUE(parser.Parse(content.c_str(), error));
    ASSERT_FALSE(parser.Supplied<username>());
}

TEST_F(CookieTest, NoCookies) {
    std::string code = R"JS(
        document.cookie
    )JS";
    ExecuteCleanJS(code, "");

    req.Get<url>() = initUrl;
    auto response =
            requestThread.Request(reqServerURL, "REQ_COOKIE_JAR", req.GetJSONString());

    const auto& content = response->WaitForMessage().content_;

    CookieParser parser;
    std::string error;
    ASSERT_TRUE(parser.Parse(content.c_str(), error));
}

TEST_F(CookieTest, InvalidURL) {
    std::string code = R"JS(
        document.cookie = 'username=Test.User';
        document.cookie
    )JS";
    ExecuteCleanJS(code, "username=Test.User");

    req.Get<url>() = "Not a url";
    auto response =
            requestThread.Request(reqServerURL, "REQ_COOKIE_JAR", req.GetJSONString());

    const auto& content = response->WaitForMessage().content_;

    CookieParser parser;
    std::string error;
    ASSERT_TRUE(parser.Parse(content.c_str(), error));
}
