
data = {
	["recvList"] = {},
	["lastSent"] = {},

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",
	
	["tosend"] = {},
	["sendpressed"] = false,
}

consts = {
	["operationTimerId"] = 3,
	
	["thinkdelay"] = 1000,
	["clickDelay"] = 200,
	["typeDelay"] = 100,
	["sendDelay"] = 1500,
}

base = {
	["senddup"] = {
		"感觉自己有点啰嗦了呢～",
		"好像不知道说点什么好了。。。",
		"我要开始复读机模式了"
	},
	["change"] = {
		"我靠，你能不能说点我能听得懂的",
		"噫"
	},
	["greet"] = {
		"打扰了，我是__AIREPLACE__，你有时间么",
		"你好，我是__AIREPLACE__",
		"我是__AIREPLACE__，有点事特地找你聊聊",
	},
	["parrotdup"] = {
		"学我说话有意思么。。。。",
		"我可能在和鹦鹉说话",
		"就这个事，不用重复啦！"
	},
	["recvdup"] = {
		"你是在自言自语吗？",
		"重要的事情不用说三遍啦。",
		"复读机，鉴定完毕。"
	}
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

playerSpoken = function(from, to, content, fromYou, toYou, groupsent)
	me:debugOutput("playerSpoken"..from..to..content)
	if fromYou then return end

	if toYou then
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
		return
	else
		-- how to qDebug()????
	end
	
	if (toSend == "") then
		toSend = getStringFromBase("change")
	end
	
	send(from, toSend)
end

timeout = function(timerid)
	if (timerid == consts.operationTimerId) then
		sendingstep()
	end
end

me:addTimer(consts.operationTimerId, 100)
