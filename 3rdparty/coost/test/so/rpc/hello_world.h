// Autogenerated.
// DO NOT EDIT. All changes will be undone.
#pragma once

#include "co/rpc.h"

namespace xx {

class HelloWorld : public rpc::Service {
  public:
    typedef std::function<void(co::Json&, co::Json&)> Fun;

    HelloWorld() {
        using std::placeholders::_1;
        using std::placeholders::_2;
        _methods["HelloWorld.hello"] = std::bind(&HelloWorld::hello, this, _1, _2);
        _methods["HelloWorld.world"] = std::bind(&HelloWorld::world, this, _1, _2);
    }

    virtual ~HelloWorld() {}

    virtual const char* name() const {
        return "HelloWorld";
    }

    virtual const co::map<const char*, Fun>& methods() const {
        return _methods;
    }

    virtual void hello(co::Json& req, co::Json& res) = 0;

    virtual void world(co::Json& req, co::Json& res) = 0;

  private:
    co::map<const char*, Fun> _methods;
};

} // xx