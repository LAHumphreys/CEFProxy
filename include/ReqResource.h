/*
 * ReqNavigate.h
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#ifndef PROXYLIBS_LIBPROXYAPP_GET_H_
#define PROXYLIBS_LIBPROXYAPP_GET_H_

#include "ProxyBrowserApp.h"
#include "ReqServer.h"

/*
 * Trigger a GET request for the specified URL.
 *
 * The difference between ReqGET and ReqNavigate is that the GET is an internal
 * retrieval (although using the shared cookie pool). This means that is will
 * *not* cause the browser window to be re-directed.
 *
 * REQUEST:
 *    {
 *         url: "https://www.some.host/target?arg=3"
 *    }
 * or
 * REQUEST:
 *    {
 *         url: "https://www.some.host/target?arg=3"
 *         postData: "{ ... }"
 *    }
 *
 * REPLY:
 *    {
 *        status: 200,
 *        document: '{ ... }'
 *    }
 *
 * NOTE: Although this is one-shot request, we must implement using the SUB
 *       protocol due to the necessity of asynchrounously bouncing:
 *         1. into the browser UI thread to allow us to do a CEF IPC call
 *         2. Then off to the renderer process to perform the get/post
 *         3. Once in the other process, we must then dispatch the request
 *            over the internet.
 *         4. ... and then back through all of that again to get back to the
 *            server thread.
 *         We don't want to lock out the RequestServer all this time.
 *
 *       This is abstracted in the JavaScript layer so that the node script
 *       driving us doesn't not need to know...
 */
class ReqGET: public SubscriptionHandler {
public:
    /**
     * Create a new handler for GET requests...
     *
     * @param server   The thread that the request server is running on.
     * @param browser  The primary (only) browser instance, which we will be
     *                 navigating
     */
    ReqGET(IPostable& server, ProxyBrowserHandler& browser);

    virtual ~ReqGET() = default;

    virtual void OnRequest(RequestHandle hdl);

    void RequestFullfilled(const size_t& id, int status, std::string data);

private:
    // ONLY TO BE USED ON THE SERVER THREAD
    class RequestStore {
    public:
        RequestStore(): idx(0) {}

        size_t StoreRequest(RequestHandle hdl);

        RequestHandle Release(const size_t& idx);

    private:
        size_t idx;
        std::map<size_t, RequestHandle> store_;
    };

    RequestStore store_;
    IPostable& server_;
    ProxyBrowserHandler& browser_;
};

#endif /* PROXYLIBS_LIBPROXYAPP_REQNAVIGATE_H_ */
