/*
 * ProxyRestEndPoint.cpp
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#include "ProxyRESTEndPoint.h"
#include <ReqNavigate.h>

#include <include/cef_life_span_handler.h>

#include "CefBaseApp.h"
#include "ReqResource.h"
#include <ReqCookieJar.h>
#include "SubOnResourceLoaded.h"

namespace {
    // TODO: This is a quick and dirty hack to get something working.
    //       It is horribly fragile - when this breaks think about how
    //       to hook in the EndPoint nicely
    class OnCreateCallBack: public CefLifeSpanHandler {
    public:
        OnCreateCallBack(ProxyRESTEndPoint& endPoint)
           : endPoint_(endPoint) {}

        /**
         * Triggered on the UI thread (and only the UI thread)
         * when a new browser has been created...
         *
         * [This "works" for us since we will never create a second browser]
         */
        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) {
            endPoint_.RunInNewThread();
        }
    private:
        ProxyRESTEndPoint& endPoint_;

        IMPLEMENT_REFCOUNTING(OnCreateCallBack);
    };
}

ProxyRESTEndPoint::ProxyRESTEndPoint(
        CefRefPtr<CefBaseApp> app,
        std::shared_ptr<ProxyBrowserHandler> browser)
   : browser_(browser)
{
    endpoint_.AddHandler("REQ_NAVIGATE", std::make_unique<ReqNavigate>(*browser));
    endpoint_.AddHandler("REQ_GET", std::make_unique<ReqGET>(endpoint_, *browser));
    endpoint_.AddHandler("SUB_LOADS", std::make_unique<SubOnResourceLoaded>(app, endpoint_));
    endpoint_.AddHandler("REQ_COOKIE_JAR", std::make_unique<ReqCookieJar> (app->GetClient(), endpoint_));


    app->GetClient()->LifeSpanHandler().InstallHandler(
            std::shared_ptr<OnCreateCallBack>(new OnCreateCallBack(*this)));
}

ProxyRESTEndPoint::~ProxyRESTEndPoint()
{
}

void ProxyRESTEndPoint::DoWork()
{
    endpoint_.HandleRequests(12345);
}
