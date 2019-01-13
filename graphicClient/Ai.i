
%module Ai

%{
#include "Ai.h"
#include "client.h"

#ifdef GRAPHICSCLIENT
#include "dialog.h"
#else
#include "console.h"
#endif

#include <QString>
%}

struct SpeakDetail;
class QString;
class Dialog;
class Client;

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


%naturalvar QStringList;

%typemap(in, checkfn = "lua_istable") QStringList
%{
for (size_t i = 0; i < lua_rawlen(L, $input); i++) {
    lua_rawgeti(L, $input, i + 1);
    const char *elem = luaL_checkstring(L, -1);
    $1 << elem;
    lua_pop(L, 1);
}
%}

%typemap(out) QStringList
%{
lua_createtable(L, $1.length(), 0);

for (int i = 0; i < $1.length(); i++) {
    QString str = $1.at(i);
    lua_pushstring(L, str.toUtf8().constData());
    lua_rawseti(L, -2, i + 1);
}

SWIG_arg++;
%}

%naturalvar SpeakDetail;

%typemap(out) SpeakDetail
%{
lua_createtable(L, 0, 6);

lua_pushstring(L, $1.from.toUtf8().constData());
lua_setfield(L, -2, "from");

lua_pushboolean(L, $1.fromYou);
lua_setfield(L, -2, "fromYou");

lua_pushboolean(L, $1.toYou);
lua_setfield(L, -2, "toYou");

lua_pushboolean(L, $1.groupSent);
lua_setfield(L, -2, "groupSent");

lua_pushunsigned(L, $1.time);
lua_setfield(L, -2, "time");

lua_pushstring(L, $1.content.toUtf8().constData());
lua_setfield(L, -2, "content");

SWIG_arg++;

%}


class Ai
{
public:
    Ai(Dialog *);
    ~Ai();

    // funcs which should be called by lua
    QString name();
    QString gender();
    QString getPlayerGender(const QString &name);
    void queryTl(const QString &id, const QString &content, const QString &key = QString(), const QString &aiComment = QString());
    void addTimer(int timerId, int timeOut);
    void killTimer(int timerId);
    QString getFirstChar(const QString &c);
    QString removeFirstChar(const QString &c);
    void debugOutput(const QString &c);
    QStringList newMessagePlayers();
    QStringList onlinePlayers();
    SpeakDetail getNewestSpokenMessage();

    void setNameCombo(const QString &name);
    void setTextFocus();
    void setText(const QString &text);
    void sendPress();
    void sendRelease();
    void sendClick();
};

%{

void setMe(lua_State *l, Ai *ai)
{
    SWIG_NewPointerObj(l, ai, SWIGTYPE_p_Ai, 0);
    lua_setglobal(l, "me");
}

%}
