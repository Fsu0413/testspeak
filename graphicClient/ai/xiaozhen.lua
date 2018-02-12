
data = {
	["recvList"] = {},
	["lastSent"] = {},
	["recvTime"] = {},
	["sentTime"] = {},
	
	["outoftime"] = false,
	["outoftimeDay"] = 0,
	["outoftimeKnown"] = {},

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",

	["tosend"] = {},
	["sendpressed"] = false,
}

consts = {
	["operationTimerId"] = 3,

	["outoftimeTimerId"] = 4,
	["outoftimeTimeout"] = 60000,

	["thinkdelay"] = 500,
	["clickDelay"] = 200,
	["typeDelay"] = 100,
	["sendDelay"] = 1500,
}

base = {
	["senddup"] = {
		"考考你，我叫什么来着？",
		"我多大了？",
		"还记得我是男生还是女生么？"
	},
	["changemale"] = {
		"顺便跟你说说，俺是大帅哥！",
		"稍微向你透露下，我是男生",
	},
	["changefemale"] = {
		"偷偷的告诉你，信不信我是大美女哦~",
		"我是女生，对我说话要温柔些~",
	},
	["greet"] = {
		"打扰了，我是__AIREPLACE__，你有时间么",
		"你好，我是__AIREPLACE__",
		"我是__AIREPLACE__，有点事特地找你聊聊",
	},
	["parrotdup"] = {
		"你叫什么来着？",
		"你多大了？",
		"你是男生还是女生啊？"
	},
	["recvdup"] = {
		"你是在自言自语吗？",
		"重要的事情不用说三遍啦。",
		"复读机，鉴定完毕。"
	},
	["outoftime"] = {
		"今天说了太多，次数用完啦。。。不得不下线了，明天继续！",
		"不好意思，没时间啦，明天有时间再聊~下线了，88~"
	},
	["revive"] = {
		"又是崭新的一天~有没有啥想跟我说说的？",
		"再次打扰啦，有时间吗~",
	},
}

generateRandom = function(rand)
	return math.random(rand * 0.3, rand * 1.5)
end

sendingstep = function()
	local timer = 1

	if (data.sendingStep ~= 5) and data.sendpressed then
		me:sendRelease()
	end

	if data.sendingStep == 0 then
		if #data.tosend ~= 0 then
			local tosend = data.tosend[1]
			data.sendingTo = tosend.to
			data.sending = tosend.content
			data.sendingStep = 1
			timer = consts.clickDelay
			table.remove(data.tosend, 1)
		end
	elseif data.sendingStep == 1 then
		-- me:popupNameCombo()
		data.sendingStep = 2
		timer = consts.clickDelay
	elseif data.sendingStep == 2 then
		me:setNameCombo(data.sendingTo)
		data.sendingStep = 3
		data.typed = ""
		timer = consts.thinkdelay
	elseif data.sendingStep == 3 then
		if data.sending == "" then
			data.sendingStep = 4
			timer = consts.sendDelay
		else
			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = consts.typeDelay
		end
	elseif data.sendingStep == 4 then
		me:sendPress()
		data.sendpressed = true
		data.sendingStep = 5
		timer = consts.clickDelay
	elseif data.sendingStep == 5 then
		me:sendClick()
		data.sendpressed = false
		data.sendingStep = 0
		timer = consts.thinkdelay
	end

	if timer ~= 1 then
		timer = generateRandom(timer)
	end

	me:addTimer(consts.operationTimerId, timer)
end

sendTo = function(to, content)
	if not to then to = "all" end
	me:debugOutput("sendTo".. to .. content)

	local tosend = {
		["to"] = to,
		["content"] = content
	}

	table.insert(data.tosend, tosend)
end

getStringFromBase = function(baseName)
	me:debugOutput("getStringFromBase" .. baseName)
	return base[baseName][math.random(1, #(base[baseName]))];
end

send = function(from, content)
	me:debugOutput("send" .. from .. content)
	data.lastSent[from] = content
	sendTo(from, content)
end

analyzeContent = function(from)
	if data.recvList[from][#(data.recvList[from])] == data.lastSent[from] then
		send(from, getStringFromBase("parrotdup"))
		return
	end
	me:debugOutput("data.queryTl" .. from .. data.recvList[from][#(data.recvList[from])])
	me:queryTl(from, data.recvList[from][#(data.recvList[from])])
end

talk = function(from, content)
	if not data.recvList[from] then
		data.recvList[from] = {}
	end

	table.insert(data.recvList[from], content)
	if #(data.recvList[from]) > 3 then
		table.remove(data.recvList[from], 1)
	end

	if data.recvTime[from] - data.sentTime[from] < 2 then
		return
	end

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

	analyzeContent(from)
end

addPlayer = function(name)
	local x = getStringFromBase("greet")
	x = string.gsub(x, "__AIREPLACE__", me:name())
	send(name, x)
end

removePlayer = function(name)
	me:debugOutput("removePlayer"..name)
	data.recvList[name] = nil
	data.lastSent[name] = nil
	data.sentTime[name] = nil
	data.recvTime[name] = nil

	if data.sendingTo == name then
		data.sending = ""
		data.sendingTo = ""
		data.sendingStep = 0
	end

	local flag = false
	while not flag do
		flag = true
		for _, i in ipairs(data.tosend) do
			if i.to == name then
				table.remove(data.tosend, _)
				flag = false
				break
			end
		end
	end
end

playerDetail = function(obname, obgender)

end

playerSpoken = function(from, to, content, fromYou, toYou, groupsent, senttime)
	me:debugOutput("playerSpoken"..from..to..content)

	if fromYou then
		data.sentTime[to] = senttime
		return
	end
	
	for _, i in ipairs(data.outoftimeKnown) do
		if i == from then
			return
		end
	end

	if toYou then
		data.recvTime[from] = senttime
		talk(from, content)
	end
end

tlReceive = function(value, sending, from)
	me:debugOutput("tlReceive" .. value .. sending)
	local toSend = ""
	if (value == 100000) or (value == 40002) then
		toSend = sending
		toSend = string.gsub(toSend, "%s", "")
		if toSend == data.lastSent then
			toSend = getStringFromBase("senddup")
		end
	elseif value == 40004 then
		toSend = getStringFromBase("outoftime")
		if not data.outoftime then
			data.outoftimeDay = os.date("*t").day
			me:addTimer(consts.outoftimeTimerId, consts.outoftimeTimeout)
			data.outoftime = true
		end
		
		local contains = false
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

	send(from, toSend)
end

timeout = function(timerid)
	if (timerid == consts.operationTimerId) then
		sendingstep()
	elseif (timerid == consts.outoftimeTimerId) then
		local dt = os.date("*t")
		local revive = false
		if (dt.day == 1) or (dt.day > data.outoftimeDay) then
			if dt.hour >= 1 then
				revive = true
			elseif dt.minute > 55 then
				revive = true
			elseif dt.minute > 5 then
				revive = (math.random(10) == 1)
			end
		end

		if revive then
			data.outoftime = false
			data.outoftimeDay = 0
			data.outoftimeKnown = {}
			if data.speakingTo ~= "" then
				for n, _ in ipairs(data.lastSent) do
					if _ then
						send(n, getStringFromBase("revive"))
					end
				end
			end
		else
			me:addTimer(consts.outoftimeTimerId, consts.outoftimeTimeout)
		end
	end
end

me:addTimer(consts.operationTimerId, 100)
