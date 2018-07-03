/*
 * ReqNavigate.cpp
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#include "ReqNavigate.h"
#include <SimpleJSON.h>

namespace
{
    NewStringField(url);
    NewStringField(status);
    typedef SimpleParsedJSON<url> Request;
    typedef SimpleParsedJSON<status> Reply;
    Request request;
    Reply reply;
}

ReqNavigate::ReqNavigate(ProxyBrowserHandler& browser)
   : browser_(browser)
{
}


ReqNavigate::~ReqNavigate() {
}

std::string ReqNavigate::OnRequest(const char* req) {
    static std::string error;
    request.Clear();
    reply.Clear();

    if ( !request.Parse(req,error) ) {
        throw InvalidRequestException{0,error};
    }

    browser_.GetBrowserSync()->GetMainFrame()->LoadURL(request.Get<url>());

    reply.Get<status>() = "DISPATCHED";
    return reply.GetJSONString();
}
