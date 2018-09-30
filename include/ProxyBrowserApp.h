/*
 * GCGVBrowserApp.h
 *
 *  Created on: 7 Feb 2015
 *      Author: lhumphreys
 */

#ifndef GCGVBROWSERAPP_H_
#define GCGVBROWSERAPP_H_
#include <future>

#include "ProxyRESTEndPoint.h"
class CefBaseApp;

#include <include/cef_browser_process_handler.h>

/**
 * Handles application level events in the UI thread (Browser). Its primary
 * responsibility is to handle the initialisation of the CEF context
 * (OnContextInitialized) in the browser process.
 *
 * All method are executed on the UI THREAD (Browser process)
 */
class ProxyBrowserHandler: public CefBrowserProcessHandler {
public:
	/**
	 * Default C'tor - nothing to do.
	 */
	ProxyBrowserHandler(CefBaseApp& app);

	/**
	 * The CEF context has been created. Create a new window and display it.
	 *
	 * This method is responsible for setting the initial paramaters of the window
	 * (including its url).
	 *
	 * Finally the method creates a new instance of GCGV_Callbacks, which is
	 * used by this window for event handling.
	 *
	 */
	virtual void OnContextInitialized() OVERRIDE;

	/**
	 * Get the a reference to the browser object
	 *
	 * NOTE: If the browser has not yet been created, this will block the
	 *       current thread until it has been created.
	 *
	 *       This must NOT be invoked on the UI thread before the browser has
	 *       been created (as that would result in a deadlock)
	 */
	virtual CefRefPtr<CefBrowser> GetBrowserSync();

	/**
	 * D'tor - nothing to do (but see reference counting...)
	 */
	virtual ~ProxyBrowserHandler();
private:
	/**
	 * Calculate the starting URL. This will default to the Content provided by the
	 * app, but can be overridden with the --url flag ( this is useful for, amongst
	 * other things, Brackets integration)
	 *
	 * @returns The url that the browser should initialise to
	 */
	std::string GetStartUrl();

	CefBaseApp& app;
	CefRefPtr<CefBrowser> browser_;

	std::promise<CefRefPtr<CefBrowser>> browserPromise_;
	std::shared_future<CefRefPtr<CefBrowser>> browserFuture_;

	// Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(ProxyBrowserHandler)
};

#endif /* GCGVBROWSERAPP_H_ */
