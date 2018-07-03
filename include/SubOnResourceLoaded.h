/*
 * SubOnResourceLoaded.h
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#ifndef PROXYLIBS_LIBPROXYAPP_SUBONRESOURCELOADED_H_
#define PROXYLIBS_LIBPROXYAPP_SUBONRESOURCELOADED_H_

#include "ReqServer.h"

#include <vector>

#include "CefBaseApp.h"
/*
 * REQUEST (empty):
 *    {
 *    }
 *
 * Update:
 *    {
 *        url: "www.google.co.uk",
 *        status: 200,
 *        source: "<html> ... </html>"
 *    }
 */
class SubOnResourceLoaded: public SubscriptionHandler {
public:
    /**
     * Create the handler, installing the CEF hook to pick up the resource load
     * notifications...
     *
     *    @param app            We require the CefBaseApp to install our hooks
     *    @param serverThread   Updates must be pushed on the server's thread
     */
    SubOnResourceLoaded(CefRefPtr<CefBaseApp> app, IPostable& serverThread);
    virtual ~SubOnResourceLoaded();

    virtual void OnRequest(RequestHandle hdl) override;

    void NotifyClients(std::string url, int code, std::string source);
private:
    std::vector<RequestHandle> activeSessions_;
    IPostable& serverThread_;
};

#endif /* PROXYLIBS_LIBPROXYAPP_SUBONRESOURCELOADED_H_ */
