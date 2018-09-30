#include <ReqCookieJar.h>
#include <SimpleJSON.h>

#include <CefBaseCookieMgr.h>

namespace
{
    NewStringField(url);
    typedef SimpleParsedJSON<url> Request;
    Request request;
}

ReqCookieJar::ReqCookieJar(CefRefPtr<CefBaseClient> client, IPostable &serverThread)
   : client(client)
   , serverThread(serverThread)
{

}

void ReqCookieJar::OnRequest(SubscriptionHandler::RequestHandle hdl) {
    request.Clear();
    std::string error;
    if (!request.Parse(hdl->RequestMessasge(), error)) {
        throw InvalidRequestException { -1, error};
    }

    client->GlobalCookieJar()->GetCookieMap(
        request.Get<url>(),
        [=] (std::shared_ptr<const CefBaseCookies::CookieNameValueMap> cookies) -> void
    {
            serverThread.PostTask([=] () -> void {
                if (hdl->Ok()) {
                    SimpleJSONBuilder builder;
                    for (const auto& pair: *cookies) {
                        const std::string& name = pair.first;
                        const std::string& value = pair.second;
                        builder.Add(name, value);
                    }

                    hdl->SendMessage(builder.GetAndClear());
                }
            });
    });

}

