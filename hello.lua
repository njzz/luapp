print("lua file loaded")

function AddNum(num1,num2)
return (num1+num2)
end

function AddString(str1,str2)
return (str1..str2)
end

function Rt3Value(v1,v2,v3)
return v1,v2,v3
end


utils={}
utils.math={}
function utils.math.add(n1,n2,n3)
	return (n1+n2+n3);
end

