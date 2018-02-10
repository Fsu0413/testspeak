
%module Ai

%{
#include "Ai.h"
#include "client.h"
#include "dialog.h"
#include <QString>
%}

%naturalvar QString;

%typemap(in,checkfn="lua_isstring") QString
%{ $1 = QString::fromUtf8(lua_tostring(L, $input)); %}

%typemap(out) QString
%{ lua_pushstring(L, $1.toUtf8().constData()); SWIG_arg++;%}

%typemap(in,checkfn="lua_isstring") const QString& ($*1_ltype temp)
%{ temp = QString::fromUtf8(lua_tostring(L, $input)); $1 = &temp; %}

%typemap(out) const QString&
%{ lua_pushstring(L, $1.toUtf8().constData()); SWIG_arg++;%}

%typemap(throws) QString, QString&, const QString&
%{ lua_pushstring(L, $1.toUtf8().constData()); SWIG_fail;%}

%typemap(throws) QString*, const QString*
%{ lua_pushstring(L, $1->toUtf8().constData()); SWIG_fail;%}

%typecheck(SWIG_TYPECHECK_STRING) QString, const QString& {
  $1 = lua_isstring(L,$input);
}

%typemap(in) QString &INPUT=const QString &;
%typemap(in, numinputs=0) QString &OUTPUT ($*1_ltype temp)
%{ $1 = &temp; %}
%typemap(argout) QString &OUTPUT
%{ lua_pushstring(L, $1->toUtf8().constData()); SWIG_arg++;%}
%typemap(in) QString &INOUT =const QString &;
%typemap(argout) QString &INOUT = QString &OUTPUT;

class QString { };
class Dialog { };
class Client { };

class Ai
{
public:
    Ai(Client *,Dialog *);
    ~Ai();

    // funcs which should be called by lua
    QString name();
    QString gender();
    void queryPlayer(const QString &name);
    void queryTl(const QString &id, const QString &content);
    void addTimer(int timerId, int timeOut);
    void killTimer(int timerId);
    bool setNameCombo(const QString &name);
    void setText(const QString &text);
    void sendPress();
    void sendRelease();
    void sendClick();
    QString getFirstChar(const QString &c);
    QString removeFirstChar(const QString &c);
    void debugOutput(const QString &c);
    void prepareExit();
};


%{

void setMe(lua_State *l, Ai *ai)
{
    SWIG_NewPointerObj(l, ai, SWIGTYPE_p_Ai, 0);
    lua_setglobal(l, "me");
}

%}