/*
 * ProxyRestEndPoint.h
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#ifndef PROXYLIBS_LIBPROXYAPP_PROXYRESTENDPOINT_H_
#define PROXYLIBS_LIBPROXYAPP_PROXYRESTENDPOINT_H_

#include "ProxyBrowserApp.h"

#include "ReqServer.h"
#include "CefBaseThread.h"
#include "CefBaseApp.h"

#include <memory>

class ProxyBrowserHandler;

class ProxyRESTEndPoint: public CefBaseThread {
public:
    ProxyRESTEndPoint(
            CefRefPtr<CefBaseApp> app,
            std::shared_ptr<ProxyBrowserHandler> browser);

    virtual ~ProxyRESTEndPoint();

private:
    /**
     * Callback to run the server...
     */
    virtual void DoWork() override;

    RequestServer endpoint_;
    std::shared_ptr<ProxyBrowserHandler> browser_;

	IMPLEMENT_REFCOUNTING(ProxyRESTEndPoint);
};

#endif /* PROXYLIBS_LIBPROXYAPP_PROXYRESTENDPOINT_H_ */
