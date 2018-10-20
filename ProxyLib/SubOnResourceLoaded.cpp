/*
 * SubOnResourceLoaded.cpp
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#include "SubOnResourceLoaded.h"

#include <SimpleJSON.h>
#include <memory>

namespace
{

    // Hook on the renderer process to notify the browser that a load has
    // completed...
    class HandleResourceLoaded: public CefLoadHandler
    {
    public:
        HandleResourceLoaded(std::function <void (std::string)> sink)
            : sink(std::move(sink)) {}
          void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override
          {
              // LoadHandler triggers on both the browser and the renderer,
              //
              // NOTE: Snapshot may only be invoked on the RENDERER!
              if (CefCurrentlyOn(TID_UI)) {
                  update.Clear();
                  update.Get<url>() = frame->GetURL();
                  update.Get<status>() = httpStatusCode;

                  sink(update.GetJSONString());
              }
          }
    private:
        std::function <void (std::string)> sink;

        // Message parser...
        NewStringField(url);
        NewIntField(status);
        SimpleParsedJSON<url, status> update;

        IMPLEMENT_REFCOUNTING(HandleResourceLoaded);
    };
}

SubOnResourceLoaded::SubOnResourceLoaded(
        CefRefPtr<CefBaseApp> app,
        IPostable& serverThread)
  : serverThread_(serverThread)
{
    // Install a handler to detect page loads...
    auto resourceLoadedHook =
            std::make_shared<HandleResourceLoaded>(
                [&] (std::string data) -> void
                {
                    this->NotifyClients(std::move(data));
                });

    app->GetClient()->LoadHandler().InstallHandler(resourceLoadedHook);
}

void SubOnResourceLoaded::OnRequest(RequestHandle hdl) {
    activeSessions_.Add(hdl);
}

void SubOnResourceLoaded::NotifyClients(std::string update) {
    serverThread_.PostTask([=] () -> void {
        activeSessions_.Publish(update);
    });
}
