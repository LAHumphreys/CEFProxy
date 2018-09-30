#ifndef CEFPROXY_REQCOOKIEJAR_H
#define CEFPROXY_REQCOOKIEJAR_H

#include "ProxyBrowserApp.h"
#include "ReqServer.h"

/*
 * Request the browser be re-pointed to a new url...
 *
 * REQUEST:
 *    {
 *         url: "https://www.google.co.uk
 *    }
 *
 * REPLY:
 *    {
 *        cookies {
 *            name1: "value 1",
 *            name2: "value 2",
 *        }
 *    }
 */
class ReqCookieJar: public SubscriptionHandler {
public:
    ReqCookieJar(
            CefRefPtr <CefBaseClient> client,
            IPostable& serverThread);

    virtual ~ReqCookieJar() = default;

    void OnRequest(RequestHandle hdl) override;

private:
    CefRefPtr<CefBaseClient> client;
    IPostable& serverThread;
};

#endif //CEFPROXY_REQCOOKIEJAR_H
