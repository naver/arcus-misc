function lua_add(a, b)
	return a + b
end

function lua_del(a,b)
	return a - b
end

function sendbox_violate()
	lua_del(1,2)
end

function sendbox_safe()
	lua_add(1,2)
end
