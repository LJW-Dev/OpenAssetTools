quickhull = {}

function quickhull:include(includes)
	if includes:handle(self:name()) then
		includedirs {
			path.join(ThirdPartyFolder(), "quickhull")
		}
	end
end

function quickhull:link(links)
	links:add(self:name())
end

function quickhull:use()
	
end

function quickhull:name()
    return "quickhull"
end

function quickhull:project()
	local folder = ThirdPartyFolder()
	local includes = Includes:create()

	project(self:name())
        targetdir(TargetDirectoryLib)
		location "%{wks.location}/thirdparty/%{prj.name}"
		kind "StaticLib"
		language "C++"
		
		files { 
			path.join(folder, "quickhull/*.hpp"),
			path.join(folder, "quickhull/*.cpp"),
			path.join(folder, "quickhull/Structs/*.hpp"),
			path.join(folder, "quickhull/Structs/*.cpp")
		}
		
		self:include(includes)

		-- Disable warnings. They do not have any value to us since it is not our code.
		warnings "off"
end
