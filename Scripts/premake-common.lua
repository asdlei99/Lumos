function addCommonXcodeBuildSettings()

	if (_OPTIONS['arch'] == "ios") then
	xcodebuildsettings
	{
		["GCC_VERSION"] = "com.apple.compilers.llvm.clang.1_0";
		["IPHONEOS_DEPLOYMENT_TARGET"] = "12.0";
		["TARGETED_DEVICE_FAMILY"]	= "1,2";
		["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES";
		["GCC_DYNAMIC_NO_PIC"] = "NO";			
		["GCC_C_LANGUAGE_STANDARD"] = "c17";			
		["CLANG_CXX_LANGUAGE_STANDARD"] = "c++17";
		["CLANG_CXX_LIBRARY"]  = "libc++";
		["ARCHS"] = "$(ARCHS_STANDARD)";
		["VALID_ARCHS"] = "$(ARCHS_STANDARD)";	
		["SDKROOT"] = "iphoneos";	
		-- this must be set to YES for all librarires and to NO for APPLICATIONS
		["SKIP_INSTALL"] = "YES";
		-- always build all arch so we dont have problems with 64
		["ONLY_ACTIVE_ARCH"] = "NO";

		-- ms disable these warnings for now 	
		["GCC_WARN_CHECK_SWITCH_STATEMENTS"] = "NO";
		["GCC_WARN_UNUSED_VARIABLE"] = "NO";

		-- ms build precompiled headers only for c++ !!!
		["GCC_PFE_FILE_C_DIALECTS"] = "c++"; 

		-- code recommnede settings 
		["CLANG_WARN_BOOL_CONVERSION"] = "YES";
		["CLANG_WARN_CONSTANT_CONVERSION"] = "YES";
		["CLANG_WARN_EMPTY_BODY"] = "YES";
		["CLANG_WARN_ENUM_CONVERSION"] = "YES";
		["CLANG_WARN_INT_CONVERSION"] = "YES";
		["CLANG_WARN_UNREACHABLE_CODE"] = "YES";
		["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES";
		["ENABLE_STRICT_OBJC_MSGSEND"] = "YES";
		["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES";
		["GCC_WARN_UNDECLARED_SELECTOR"] = "YES";
		["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES";
		["GCC_WARN_UNUSED_FUNCTION"] = "YES";
	}
	elseif (_OPTIONS['arch'] == "tvos") then
	
	buildoptions {"-fembed-bitcode"}

	xcodebuildsettings
	{
		["GCC_VERSION"] = "com.apple.compilers.llvm.clang.1_0";
		["TVOS_DEPLOYMENT_TARGET"] = "9.0";
		["TARGETED_DEVICE_FAMILY"]	= "3";
		["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES";
		["GCC_DYNAMIC_NO_PIC"] = "NO";			
		["GCC_C_LANGUAGE_STANDARD"] = "c17";			
		["CLANG_CXX_LANGUAGE_STANDARD"] = "c++17";
		["CLANG_CXX_LIBRARY"]  = "libc++";
		["ARCHS"] = "arm64";
		["VALID_ARCHS"] = "arm64";	
		["SDKROOT"] = "appletvos";	
		-- this must be set to YES for all librarires and to NO for APPLICATIONS
		["SKIP_INSTALL"] = "YES";
		-- always build all arch so we dont have problems with 64
		["ONLY_ACTIVE_ARCH"] = "NO";

		-- ms disable these warnings for now 	
		["GCC_WARN_CHECK_SWITCH_STATEMENTS"] = "NO";
		["GCC_WARN_UNUSED_VARIABLE"] = "NO";

		-- ms build precompiled headers only for c++ !!!
		["GCC_PFE_FILE_C_DIALECTS"] = "c++"; 

		-- code recommnede settings 
		["CLANG_WARN_BOOL_CONVERSION"] = "YES";
		["CLANG_WARN_CONSTANT_CONVERSION"] = "YES";
		["CLANG_WARN_EMPTY_BODY"] = "YES";
		["CLANG_WARN_ENUM_CONVERSION"] = "YES";
		["CLANG_WARN_INT_CONVERSION"] = "YES";
		["CLANG_WARN_UNREACHABLE_CODE"] = "YES";
		["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES";
		["ENABLE_STRICT_OBJC_MSGSEND"] = "YES";
		["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES";
		["GCC_WARN_UNDECLARED_SELECTOR"] = "YES";
		["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES";
		["GCC_WARN_UNUSED_FUNCTION"] = "YES";
	}
	
	elseif (_OPTIONS['arch'] == "macosx") then
	xcodebuildsettings
	{
		["GCC_VERSION"] = "com.apple.compilers.llvm.clang.1_0";
		["GCC_SYMBOLS_PRIVATE_EXTERN"] = "YES";
		["GCC_DYNAMIC_NO_PIC"] = "NO";			
		["GCC_C_LANGUAGE_STANDARD"] = "c11";			
		["CLANG_CXX_LANGUAGE_STANDARD"] = "c++11";
		["CLANG_CXX_LIBRARY"]  = "libc++";		
		["SDKROOT"] = "macosx";	
	}
	end
end	