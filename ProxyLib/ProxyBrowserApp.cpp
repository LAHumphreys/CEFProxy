/*
 * GCGVBrowserApp.cpp
 *
 *  Created on: 7 Feb 2015
 *      Author: lhumphreys
 */

#include <CefBaseApp.h>
#include <memory>

#include "include/wrapper/cef_helpers.h"
#include "ProxyBrowserApp.h"

ProxyBrowserHandler::ProxyBrowserHandler(CefBaseApp& _app)
    : app(_app)
    , browser_(nullptr)
    , browserFuture_(browserPromise_.get_future())
{
}

ProxyBrowserHandler::~ProxyBrowserHandler() {
}

void ProxyBrowserHandler::OnContextInitialized() {
	// This should be called from the browser process..
	CEF_REQUIRE_UI_THREAD();

	// Information used when creating the native window.
	CefWindowInfo window_info;

	// Specify CEF browser settings here.
	CefBrowserSettings browser_settings;

	std::string url = GetStartUrl();

	// Create the first browser window.
    browser_ = CefBrowserHost::CreateBrowserSync(
        window_info,
        app.GetClient(),
        url,
        browser_settings,
        NULL); // Use the global context - important as the URL requests will be doing the same...

    browserPromise_.set_value(browser_);
}

CefRefPtr<CefBrowser> ProxyBrowserHandler::GetBrowserSync() {
    return browserFuture_.get();
}

std::string ProxyBrowserHandler::GetStartUrl() {
	std::string url;

	// Check if a "--url=" value was provided via the command-line. If so, use
	// that instead of the default URL.
	CefRefPtr<CefCommandLine> command_line =
			CefCommandLine::GetGlobalCommandLine();

	url = command_line->GetSwitchValue("url");

	if (url.empty()) {
		url = "http://www.bbc.co.uk";
	}

	return url;
}
