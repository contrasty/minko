--[[
Copyright (c) 2013 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
]]--

-- html-overlay plugin
minko.plugin["html-overlay"] = {}

minko.plugin["html-overlay"].enable = function()
	minko.plugin.links { "html-overlay" }
	defines { "MINKO_PLUGIN_HTML_OVERLAY" }
	includedirs {
		minko.plugin.path("html-overlay") .. "/include"
	}

	prelinkcommands {
		minko.action.copy(path.join(minko.plugin.path("html-overlay"), "asset"))
	}

	minko.plugin.enable("sdl")
	--minko.plugin.enable("lua")

	configuration { "html5", "ConsoleApp or WindowedApp" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset")
		}

	configuration { "ios or osx64" }
		buildoptions { "-x objective-c++" }

	configuration { "ios", "ConsoleApp or WindowedApp" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/WebViewJavascriptBridge/WebViewJavascriptBridge.js.txt"),
		}

	configuration { "android", "SharedLib" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset")
		}

	configuration { "osx64", "ConsoleApp or WindowedApp" }
		links { "WebKit.framework" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/WebViewJavascriptBridge/WebViewJavascriptBridge.js.txt"),
		}

	configuration { "windows32 or windows64", "ConsoleApp or WindowedApp" }
		links { "libcef" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/resource/*"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/win/dll/*.dll")
		}

	configuration { "windows32 or windows64", "debug", "ConsoleApp or WindowedApp"}
		libdirs { minko.plugin.path("html-overlay") .. "/lib/win/debug" }

	configuration { "windows32 or windows64", "release", "ConsoleApp or WindowedApp"}
		libdirs { minko.plugin.path("html-overlay") .. "/lib/win/release" }

	configuration { "linux32 or linux64"}
		buildoptions {
			"-pthread"
		}

	configuration { "linux32 or linux64", "ConsoleApp or WindowedApp" }
		linkoptions {
			"-Wl,-rpath,."
		}
		links { "cef" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/asset"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/resource/locales"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/resource/cef.pak"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/resource/devtools_resources.pak")
		}

	configuration { "linux32", "ConsoleApp or WindowedApp" }
		libdirs { minko.plugin.path("html-overlay") .. "/lib/linux32/" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux32/libcef.so"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux32/libffmpegsumo.so"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux32/chrome-sandbox")
		}

	configuration { "linux64", "ConsoleApp or WindowedApp" }
		libdirs { minko.plugin.path("html-overlay") .. "/lib/linux64/" }
		prelinkcommands {
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux64/libcef.so"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux64/libffmpegsumo.so"),
			minko.action.copy(minko.plugin.path("html-overlay") .. "/lib/linux64/chrome-sandbox")
		}
end

minko.plugin["html-overlay"].dist = function(pluginDistDir)
	configuration { "windows32 or windows64" }
		os.mkdir(pluginDistDir .. "/lib/win/")
		minko.os.copyfiles(minko.plugin.path("html-overlay") .. "/lib/win/", pluginDistDir .. "/lib/win/")

	configuration { "linux32 or linux64" }
		os.mkdir(pluginDistDir .. "/lib/lin/")
		os.mkdir(pluginDistDir .. "/lib/resource/")
		minko.os.copyfiles(minko.plugin.path("html-overlay") .. "/lib/linux32/", pluginDistDir .. "/lib/linux32/")
		minko.os.copyfiles(minko.plugin.path("html-overlay") .. "/lib/linux64/", pluginDistDir .. "/lib/linux64/")
		minko.os.copyfiles(minko.plugin.path("html-overlay") .. "/lib/resource/", pluginDistDir .. "/lib/resource/")
end

newoption {
	trigger			= "with-html-overlay",
	description		= "Enable the Minko HTML Overlay plugin."
}
