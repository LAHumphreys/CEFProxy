/*
 * ReqNavigate.h
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#ifndef PROXYLIBS_LIBPROXYAPP_REQNAVIGATE_H_
#define PROXYLIBS_LIBPROXYAPP_REQNAVIGATE_H_

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
 *        status: 'DISPATCHED'
 *    }
 */
class ReqNavigate: public RequestReplyHandler {
public:
    /**
     * Create a new handler for navigation requests...
     *
     * @param browser  The primary (only) browser instance, which we will be
     *                 navigating
     */
    ReqNavigate(ProxyBrowserHandler& browser);

    virtual ~ReqNavigate();

    virtual std::string OnRequest(const char* req);

private:
    ProxyBrowserHandler& browser_;
};

#endif /* PROXYLIBS_LIBPROXYAPP_REQNAVIGATE_H_ */
