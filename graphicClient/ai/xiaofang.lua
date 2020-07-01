
local AiCommon = require("AiCommon")

local localFunc = {}

data = {
	["recvList"] = {},
	["lastSent"] = {},
	["recvContent"] = {},
	["recvContentNotReplyed"] = {},

	["outoftime"] = false,
	["outoftimeDay"] = 0,
	["outoftimeKnown"] = {},

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",

	["sendpressed"] = false,
	["toview"] = {},
	["currentViewing"] = {},
}

consts = {
	["detectLostMessageTimerId"] = AiCommon.UserTimerId + 1,
	["detectLostMessageTimerInterval"] = 45000,
	["operationTimerId"] = AiCommon.UserTimerId + 3,
	["outoftimeTimerId"] = AiCommon.UserTimerId + 4,

	["boringKey"] = {
		"无聊",
		"难受",
		"寂寞",
		"一个人",
		"聊天",
		"撩",
		"尬"
	},

}

base = {
	["senddup"] = {
		"考考你，我叫什么来着？",
		"我多大了？",
		"还记得我是男生还是女生么？"
	},
	["changemale"] = {
		"顺便跟你说说，俺是个帅哥！",
		"稍微向你透露下，我是男生",
	},
	["changefemale"] = {
		"偷偷的告诉你，信不信我是大美女哦~",
		"我是女生，对我说话要温柔些~",
	},
	["callme"] = {
		"叫我吗？",
		"您在找我么",
		"需不需要我给您排忧解难？",
	},
	["greetfemale"] = {
		"我是__AIREPLACE__，看您有点无聊，我来陪您好了",
		"这么无聊，怕不是有什么心事？",
	},
	["greetmale"] = {
		"俺是__AIREPLACE__，想俺了么？",
		"看您这么无聊，是不是想俺想到流鼻血。"
	},
	["parrotdup"] = {
		"你叫什么来着？",
		"你多大了？",
		"你是男生还是女生啊？"
	},
	["outoftime"] = {
		"今天说了太多，次数用完啦。。。不得不下线了，明天继续！",
		"不好意思，没时间啦，明天有时间再聊~下线了，88~"
	},
}

sendingstep = function()
	local timer = 100

	if data.sendingStep == 0 then
		if #data.toview ~= 0 then
			local toview = data.toview[1]
			data.currentViewing = toview
			data.sendingStep = 101
			timer = AiCommon.TimerConsts.sendDelay
			table.remove(data.toview, 1)
		end
	elseif data.sendingStep == 101 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
		else
			data.sendingStep = 102
			me:setNameCombo(data.currentViewing.name)
		end
	elseif data.sendingStep == 102 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			timer = AiCommon.TimerConsts.sendDelay
		else
			if not data.currentViewing.time then
				data.currentViewing.time = os.time()
				localFunc.messageDetail1(me:getNewestSpokenMessage())
			elseif data.currentViewing.content then
				local passedTime = os.difftime(os.time(), data.currentViewing.time)
				data.sendingStep = 103
				timer = math.max(AiCommon.TimerConsts.thinkDelay - passedTime * 1000 - AiCommon.TimerConsts.sendDelay, 100)
			else
				local passedTime = os.difftime(os.time(), data.currentViewing.time)
				if passedTime >= 30 then
					data.currentViewing.cancel = true
				end
			end
		end
	elseif data.sendingStep == 103 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
		else
			data.sending = data.currentViewing.content
			data.typed = ""
			me:setTextFocus()
			data.sendingStep = 3
			timer = AiCommon.TimerConsts.sendDelay
		end
	elseif data.sendingStep == 3 then
		if data.sending == "" then
			if data.currentViewing.cancel then
				data.sendingStep = 77
				timer = AiCommon.TimerConsts.cancelDelay
			else
				data.sendingStep = 4
				timer = AiCommon.TimerConsts.sendDelay
			end
		else
			if data.currentViewing.cancel then
				data.sendingStep = 76
			end

			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = AiCommon.TimerConsts.typeDelay
		end
	elseif data.sendingStep == 4 then
		if data.currentViewing.cancel then
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			me:sendPress()
			data.sendpressed = true
			data.sendingStep = 5
			timer = AiCommon.TimerConsts.clickDelay
		end
	elseif data.sendingStep == 5 then
		if data.currentViewing.cancel then
			me:sendRelease()
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			me:sendClick()
			data.sendingStep = 0
			data.currentViewing = {}
			timer = AiCommon.TimerConsts.sendDelay
		end
		data.sendpressed = false
	elseif data.sendingStep == 76 then
		if (data.sending == "") or (math.random(0, 9) < 1) then
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = AiCommon.TimerConsts.typeDelay
		end
	elseif data.sendingStep == 77 then
		data.sending = ""
		data.typed = ""
		me:setTextFocus()
		data.sendingStep = 78
		timer = AiCommon.TimerConsts.sendDelay
	elseif data.sendingStep == 78 then
		me:setText("")
		data.currentViewing = {}
		data.sendingStep = 0
	end

	timer = AiCommon.generateRandom(timer)

	me:addTimer(consts.operationTimerId, timer)
end

cancelAllPendingSend = function(to)
	local exist = true
	while exist do
		exist = false
		for _, p in ipairs(data.toview) do
			if p.name == to then
				table.remove(data.toview, _)
				exist = true
				break
			end
		end
	end
end

sendTo = function(to, content, isFromTl)
	if not to then to = "all" end
	
	if to ~= "all" then
		data.recvContent[to] = {
			time = data.recvContentNotReplyed[to].time,
			content = data.recvContentNotReplyed[to].content,
		}
	end
	
	me:debugOutput("sendTo".. to .. content)
	cancelAllPendingSend(to)

	if (data.currentViewing.name == to) and (data.sendingStep == 102) and (not data.currentViewing.cancel) then
		data.currentViewing.content = content
		data.currentViewing.contentIsFromTl = isFromTl
	else
		local x = {
			["name"] = to,
			["content"] = content,
			["contentIsFromTl"] = isFromTl,
		}
		table.insert(data.toview, x)
	end
end

getStringFromBase = function(baseName)
	me:debugOutput("getStringFromBase" .. baseName)
	return base[baseName][math.random(1, #(base[baseName]))];
end

send = function(from, content, isFromTl)
	me:debugOutput(tostring(isFromTl) .. "send" .. from .. content)
	data.lastSent[from] = content
	sendTo(from, content, isFromTl)
end

analyzeContent = function(from)
	if data.recvList[from][#(data.recvList[from])] == data.lastSent[from] then
		send(from, getStringFromBase("parrotdup"), false)
		return
	end
	me:debugOutput("data.queryTl" .. from .. data.recvList[from][#(data.recvList[from])])
	me:queryTl(from, data.recvList[from][#(data.recvList[from])])
	return true
end

talk = function(from, content)
	if not data.recvList[from] then
		data.recvList[from] = {}
	end

	table.insert(data.recvList[from], content)
	if #(data.recvList[from]) > 3 then
		table.remove(data.recvList[from], 1)
	end

	if (data.sendingStep ~= 102) and (data.currentViewing.name == from) then
		data.currentViewing.cancel = true
	end

local judgeIgnore = function()
	if #(data.recvList[from]) == 3 then
		local allequal = true;
		local first
		for _, i in ipairs(data.recvList[from]) do
			if not first then
				first = i
			elseif first ~= i then
				allequal = false
				break
			end
		end
		if allequal then return end
	end

	return true
end

	if (data.sendingStep == 102) and (data.currentViewing.name == from) and (not data.currentViewing.cancel) then
		if not judgeIgnore() then
			data.currentViewing.cancel = true
		else
			return analyzeContent(from)
		end
	else
		if judgeIgnore() then
			return analyzeContent(from)
		end
	end
end

AiCommon.Callbacks.removePlayer = function(name)
	me:debugOutput("removePlayer"..name)
	data.recvList[name] = nil
	data.lastSent[name] = nil
	data.recvContent[name] = nil

	if data.currentViewing.name == name then
		data.currentViewing.cancel = true
	end

	cancelAllPendingSend(name)
end 

local messageReceived = function(from)
	if from == data.currentViewing.name then
		data.currentViewing.cancel = true
		table.insert(data.toview, 1, {name=from})
	else
		table.insert(data.toview, {name=from})
	end
end

AiCommon.Callbacks.messageReceived = function(from)
	me:debugOutput("messageReceived"..from)
	messageReceived(from)
end


AiCommon.Callbacks.messageDetail = function(detail)
	if detail.fromYou then
		return
	end

	if (not data.currentViewing.name)
			or ((data.currentViewing.name == "all") and detail.groupSent)
			or ((data.currentViewing.name == detail.from) and (not detail.groupSent)) then
		messageReceived(detail.groupSent and "all" or detail.from)
	end
end

--AiCommon.Callbacks.messageDetail = function(detail)
localFunc.messageDetail1 = function(detail)
local playerSpoken1 = function(from, content, fromYou, toYou, groupsent, sendtime)
	me:debugOutput("playerSpoken"..from..content)

	if fromYou or (sendtime == 0) then
		return
	end

	local relatedPerson = from
	if groupSent then
		relatedPerson = "all"
	end
	local recvContent = data.recvContent[relatedPerson]
	if recvContent and (recvContent.time == sendtime) and (recvContent.content == content) then
		return
	end
	if relatedPerson == "all" then
		data.recvContent[relatedPerson] = {
			time = sendtime,
			content = content
		}
	else
		data.recvContentNotReplyed[relatedPerson] = {
			time = sendtime,
			content = content
		}
	end

	for _, i in ipairs(data.outoftimeKnown) do
		if i == from then
			return
		end
	end

	if toYou then
		local r = talk(from, content)
		return from, r
	end

	if groupsent then
		if string.find(content, me:name()) then
			send(from, getStringFromBase("callme"), false)
			return from
		else
			local boring = false
			for _, i in ipairs(consts.boringKey) do
				if string.find(content, i) then
					boring = true
					break
				end
			end

			if boring then
				local sending = getStringFromBase("greet" .. me:gender())
				sending = string.gsub(sending, "__AIREPLACE__", me:name())
				send(from, sending, false)
				return from
			end
		end
	end
end
	if (data.sendingStep == 102) and (not data.currentViewing.cancel) then
		local willSpeak, async = playerSpoken1(detail.from, detail.content, detail.fromYou, detail.toYou, detail.groupSent, detail.time)
		if not willSpeak then
			if not data.currentViewing.content then
				data.currentViewing.cancel = true
			end
		else
			if data.currentViewing.content and async then
				data.currentViewing.content = nil
			end
			if willSpeak ~= data.currentViewing.name then
				data.currentViewing.cancel = true
			end
		end
	end

end

tlReceive = function(value, sending, from)
local tlReceive1 = function(value, sending, from)
	local isFromTl = false
	me:debugOutput("tlReceive" .. value .. sending)
	local toSend = ""
	if (value == 100000) or (value == 40002) then
		toSend = sending
		if toSend == data.lastSent[from] then
			toSend = getStringFromBase("senddup")
		else
			isFromTl = true
		end
	elseif value == 40004 then
		toSend = getStringFromBase("outoftime")
		if not data.outoftime then
			data.outoftimeDay = os.date("*t").day
			me:addTimer(consts.outoftimeTimerId, AiCommon.TimerConsts.outoftimeTimeout)
			data.outoftime = true
		end

		for _, i in ipairs(data.outoftimeKnown) do
			if i == from then
				return
			end
		end
		table.insert(data.outoftimeKnown, from)
	else
		-- how to qDebug()????
	end

	if (toSend == "") then
		toSend = getStringFromBase("change" .. me:gender())
	end
	return toSend, isFromTl
end
	local toSend, isFromTl = tlReceive1(value, sending, from)
	if toSend then
		send(from, toSend, isFromTl)
	elseif (data.sendingStep == 102) and (data.currentViewing.name == from) and (not data.currentViewing.cancel) then
		data.currentViewing.cancel = true
	end
end

timeout = function(timerid)
	if (timerid == consts.operationTimerId) then
		sendingstep()
	elseif (timerid == consts.outoftimeTimerId) then
		local dt = os.date("*t")
		me:debugOutput("outoftimeTimerId " .. dt.day .. " " .. dt.hour .. " " .. dt.min.. " " .. dt.sec)
		local revive = false
		if dt.day ~= data.outoftimeDay then
			if dt.hour >= 1 then
				revive = true
			elseif dt.min > 55 then
				revive = true
			elseif dt.min > 5 then
				revive = (math.random(500) == 1)
			end
		end

		if revive then
			data.outoftime = false
			data.outoftimeDay = 0
			data.outoftimeKnown = {}
		else
			me:addTimer(consts.outoftimeTimerId, AiCommon.TimerConsts.outoftimeTimeout)
		end
	elseif (timerid == consts.detectLostMessageTimerId) then
		local contains = function(t, it)
			for _, i in ipairs(t) do
				if i == it then
					return true
				end
			end
			return false
		end
		
		local tab = {}
		for k, _ in pairs(data.recvList) do
			if not contains(tab, k) then
				table.insert(tab, k)
			end
		end
		for k, _ in pairs(data.lastSent) do
			if not contains(tab, k) then
				table.insert(tab, k)
			end
		end
		for k, _ in pairs(data.recvContent) do
			if not contains(tab, k) then
				table.insert(tab, k)
			end
		end
		
		for _, i in ipairs(tab) do
			table.insert(data.toview, {name=i})
		end
		me:addTimer(consts.detectLostMessageTimerId, consts.detectLostMessageTimerInterval)
	else
		AiCommon.timeout(timerid)
	end
end

math.randomseed(os.time())
me:addTimer(consts.operationTimerId, 100)
me:addTimer(consts.detectLostMessageTimerId, consts.detectLostMessageTimerInterval)
