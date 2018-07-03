#include "CefBaseApp.h"
#include "CefBaseMainLoop.h"

#include <thread>
#include <chrono>

#include "ProxyBrowserApp.h"
#include "CefBaseDefaults.h"

// Entry point function for all processes.
int main(int argc, char* argv[]) {
  
	// SimpleApp implements application-level callbacks. It will create the first
	// browser instance in OnContextInitialized() after CEF has initialized.
    CefRefPtr<CefBaseApp> app(new CefBaseApp);

    CefBaseDefaults::InstallDefaultHandlers(*app);

    std::shared_ptr<ProxyBrowserHandler> browser(new ProxyBrowserHandler(*app.get()));

    std::shared_ptr<ProxyRESTEndPoint> restEndPoint(new ProxyRESTEndPoint(app, browser));

    app->Browser().InstallHandler(browser);

	return CefBaseAppUtils::Main(argc, argv, app);
}
