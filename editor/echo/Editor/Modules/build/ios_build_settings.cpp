#include "ios_build_settings.h"
#include <engine/core/util/PathUtil.h>
#include <engine/core/io/IO.h>
#include <engine/core/main/Engine.h>
#include <thirdparty/pugixml/pugixml.hpp>
#include <QProcess>
#include "ImageHelper.h"
#include <engine/core/main/module.h>
#include <engine/core/io/stream/FileHandleDataStream.h>

namespace Echo
{
    iOSBuildSettings::AppIconItem::AppIconItem(i32 size)
        : m_size(size)
    {
        
    }

    iOSBuildSettings::LaunchImageItem::LaunchImageItem(i32 width, i32 height, i32 scale, iOSBuildSettings::DeviceType type)
        : m_width(width)
        , m_height(height)
        , m_scale(scale)
        , m_type(type)
    {}

    String iOSBuildSettings::LaunchImageItem::getPortraitPath() const
    {
        return "    ${MODULE_PATH}/frame/Platform/iOS/Launch/" + getPortraitName();
    }

    String iOSBuildSettings::LaunchImageItem::getLandscapePath() const
    {
        return "    ${MODULE_PATH}/frame/Platform/iOS/Launch/" + getLandscapeName();
    }

    String iOSBuildSettings::LaunchImageItem::getPortraitName() const
    {
        i32 width = std::min<i32>(m_width, m_height);
        i32 height = std::max<i32>(m_width, m_height);
        return StringUtil::Format("Default-Portrait-%dx%d@%dx.png", width, height, m_scale);
    }

    String iOSBuildSettings::LaunchImageItem::getLandscapeName() const
    {
        i32 width = std::max<i32>(m_width, m_height);
        i32 height = std::min<i32>(m_width, m_height);
        return StringUtil::Format("Default-Landscape-%dx%d@%dx.png", width, height, m_scale);
    }

    iOSBuildSettings::iOSBuildSettings()
    {
        configAppIcons();
        configLaunchImages();
    }

    iOSBuildSettings::~iOSBuildSettings()
    {

    }

    iOSBuildSettings* iOSBuildSettings::instance()
    {
        static iOSBuildSettings* inst = EchoNew(iOSBuildSettings);
        return inst;
    }

    void iOSBuildSettings::bindMethods()
    {
        CLASS_BIND_METHOD(iOSBuildSettings, getAppName);
        CLASS_BIND_METHOD(iOSBuildSettings, setAppName);
        CLASS_BIND_METHOD(iOSBuildSettings, getIdentifier);
        CLASS_BIND_METHOD(iOSBuildSettings, setIdentifier);
        CLASS_BIND_METHOD(iOSBuildSettings, getVersion);
        CLASS_BIND_METHOD(iOSBuildSettings, setVersion);
        CLASS_BIND_METHOD(iOSBuildSettings, getIconRes);
        CLASS_BIND_METHOD(iOSBuildSettings, setIconRes);
        CLASS_BIND_METHOD(iOSBuildSettings, isHiddenStatusBar);
        CLASS_BIND_METHOD(iOSBuildSettings, setHiddenStatusBar);

        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "AppName",    Variant::Type::String,          getAppName,       setAppName);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "Identifier", Variant::Type::String,          getIdentifier,    setIdentifier);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "Version",    Variant::Type::String,          getVersion,       setVersion);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "Icon",       Variant::Type::ResourcePath,    getIconRes,       setIconRes);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "HiddenStatusBar", Variant::Type::Bool,       isHiddenStatusBar,setHiddenStatusBar);
        
        // Ui interface orientation
        CLASS_BIND_METHOD(iOSBuildSettings, isUIInterfaceOrientationPortrait);
        CLASS_BIND_METHOD(iOSBuildSettings, setUIInterfaceOrientationPortrait);
        CLASS_BIND_METHOD(iOSBuildSettings, isUIInterfaceOrientationPortraitUpsideDown);
        CLASS_BIND_METHOD(iOSBuildSettings, setUIInterfaceOrientationPortraitUpsideDown);
        CLASS_BIND_METHOD(iOSBuildSettings, isUIInterfaceOrientationLandscapeLeft);
        CLASS_BIND_METHOD(iOSBuildSettings, setUIInterfaceOrientationLandscapeLeft);
        CLASS_BIND_METHOD(iOSBuildSettings, isUIInterfaceOrientationLandscapeRight);
        CLASS_BIND_METHOD(iOSBuildSettings, setUIInterfaceOrientationLandscapeRight);
        
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "Portrait",           Variant::Type::Bool, isUIInterfaceOrientationPortrait,            setUIInterfaceOrientationPortrait);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "PortraitUpsideDown", Variant::Type::Bool, isUIInterfaceOrientationPortraitUpsideDown,  setUIInterfaceOrientationPortraitUpsideDown);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "LandscapeLeft",      Variant::Type::Bool, isUIInterfaceOrientationLandscapeLeft,       setUIInterfaceOrientationLandscapeLeft);
        CLASS_REGISTER_PROPERTY(iOSBuildSettings, "LandscapeRight",     Variant::Type::Bool, isUIInterfaceOrientationLandscapeRight,      setUIInterfaceOrientationLandscapeRight);
        
        CLASS_REGISTER_PROPERTY_HINT(iOSBuildSettings, "Portrait",           PropertyHintType::Category, "Device Orientation");
        CLASS_REGISTER_PROPERTY_HINT(iOSBuildSettings, "PortraitUpsideDown", PropertyHintType::Category, "Device Orientation");
        CLASS_REGISTER_PROPERTY_HINT(iOSBuildSettings, "LandscapeLeft",      PropertyHintType::Category, "Device Orientation");
        CLASS_REGISTER_PROPERTY_HINT(iOSBuildSettings, "LandscapeRight",     PropertyHintType::Category, "Device Orientation");
    }

	ImagePtr iOSBuildSettings::getPlatformThumbnail() const
	{
		return Image::loadFromFile(Engine::instance()->getRootPath() + "editor/echo/Editor/Modules/build/editor/icon/ios.png");
	}

    void iOSBuildSettings::configAppIcons()
    {
        m_appIcons.emplace_back(20);
        m_appIcons.emplace_back(29);
        
        m_appIcons.emplace_back(40);
        m_appIcons.emplace_back(58);
        m_appIcons.emplace_back(60);
        m_appIcons.emplace_back(76);
        
        m_appIcons.emplace_back(80);
        m_appIcons.emplace_back(87);
        
        m_appIcons.emplace_back(120);
        m_appIcons.emplace_back(152);
        m_appIcons.emplace_back(167);
        m_appIcons.emplace_back(180);
        
        m_appIcons.emplace_back(1024);
    }

    void iOSBuildSettings::configLaunchImages()
    {
        // https://developer.apple.com/design/human-interface-guidelines/ios/icons-and-images/launch-screen/
        
        // iphone se
        m_launchImages.emplace_back(320, 568, 2, DeviceType::iPhone);
        
        // iphone6s,iphone7,iphone8
        m_launchImages.emplace_back(375, 667, 2, DeviceType::iPhone);
        
        // iphone6s plus, iphone7 plus, iphone8 plus
        m_launchImages.emplace_back(414, 736, 3, DeviceType::iPhone);
        
        // iphoneX, iphoneXs 1125px × 2436px
        m_launchImages.emplace_back(375, 812, 3, DeviceType::iPhone);
        
        // iphone XR
        m_launchImages.emplace_back(414, 896, 2, DeviceType::iPhone);
        
        // iphone Xs Max
        m_launchImages.emplace_back(414, 896, 3, DeviceType::iPhone);
        
        // ipad mini, ipad
        m_launchImages.emplace_back(1024, 768, 2, DeviceType::iPad);
        
        // 10.5 iPad pro
        m_launchImages.emplace_back(1112, 834, 2, DeviceType::iPad);
        
        // 11 iPad pro
        m_launchImages.emplace_back(1194, 834, 2, DeviceType::iPad);
    
        // 12.9 iPad pro
        m_launchImages.emplace_back(1366, 1024, 2, DeviceType::iPad);
    }
    
    void iOSBuildSettings::setOutputDir(const String& outputDir)
    {
        m_outputDir = outputDir;
        PathUtil::FormatPath(m_outputDir, false);
    }

    bool iOSBuildSettings::prepare()
    {
        m_rootDir   = Engine::instance()->getRootPath();
        m_projectDir = Engine::instance()->getResPath();
        m_outputDir = m_outputDir.empty() ? PathUtil::GetCurrentDir() + "/build/ios/" : m_outputDir;
        m_solutionDir = m_outputDir + "xcode/";
        
        // delete output dir
        if(PathUtil::IsDirExist(m_outputDir))
        {
            PathUtil::DelPath(m_outputDir);
        }

        // create dir
        if(!PathUtil::IsDirExist(m_outputDir))
        {
            log("Create output directory : [%s]", m_outputDir.c_str());
            PathUtil::CreateDir(m_outputDir);
        }
        
        return true;
    }

    void iOSBuildSettings::setIconRes(const ResourcePath& path)
    {

    }

    void iOSBuildSettings::copySrc()
    {
        log("Copy Engine Source Code ...");

        // copy app
        PathUtil::CopyDir( m_rootDir + "app/ios/", m_outputDir + "app/ios/");

        // copy engine
        PathUtil::CopyDir( m_rootDir + "engine/", m_outputDir + "engine/");

        // copy thirdparty
        PathUtil::CopyDir( m_rootDir + "thirdparty/", m_outputDir + "thirdparty/");

        // copy CMakeLists.txt
        PathUtil::CopyFilePath( m_rootDir + "CMakeLists.txt", m_outputDir + "CMakeLists.txt");
        
        // copy build script
        PathUtil::CopyFilePath( m_rootDir + "tool/build/ios/cmake.sh", m_outputDir + "cmake.sh");
    }

    void iOSBuildSettings::copyRes()
    {
        log("Convert Project File ...");

        // copy res
        PathUtil::CopyDir( m_projectDir, m_outputDir + "app/ios/resources/data/");
        packageRes(m_outputDir + "app/ios/resources/data/");

        // rename
        String projectFile = PathUtil::GetPureFilename( Engine::instance()->getConfig().m_projectFile);
        PathUtil::RenameFile(m_outputDir + "app/ios/resources/data/" + projectFile, m_outputDir + "app/ios/resources/data/app.echo");
    }

    bool iOSBuildSettings::rescaleIcon( const char* iFilePath, const char* oFilePath, ui32 targetWidth, ui32 targetHeight)
     {
         ImageHelper helper;
         return  helper.rescaleImage(iFilePath,oFilePath,targetWidth,targetHeight);
     }

    void iOSBuildSettings::replaceIcon()
    {
        String iconFullPath = IO::instance()->convertResPathToFullPath( m_iconRes.getPath());
        if(PathUtil::IsFileExist(iconFullPath))
        {
            for(const AppIconItem& item : m_appIcons)
            {
                String outputPath = m_outputDir + StringUtil::Format("app/ios/Assets.xcassets/AppIcon.appiconset/Icon%dx%d.png", item.m_size, item.m_size);
                PathUtil::DelPath(outputPath);
                
                rescaleIcon( iconFullPath.c_str(), outputPath.c_str(), item.m_size, item.m_size);
            }
        }
    }

    void iOSBuildSettings::replaceLaunchImage()
    {
        
    }

    void iOSBuildSettings::cmake()
    {
        String workingDir = m_outputDir + "build/ios/";
        String cmakeCmd = "sh cmake.sh";
        if(!m_listener->onExecCmd(cmakeCmd.c_str(), workingDir.c_str()))
        {
            log("exec command [%s] failed.", cmakeCmd.c_str());
        }
    }

    void iOSBuildSettings::compile()
    {

    }

    void iOSBuildSettings::build()
    {
        log("Build App for iOS platform.");

        m_listener->onBegin();

        if(prepare())
        {
            copySrc();
            copyRes();
            
            replaceIcon();
            replaceLaunchImage();
            
            // overwrite config
            writeInfoPlist();
            writeCMakeList();

			writeModuleConfig();

            //cmake();

            //compile();
        }

        m_listener->onEnd();
    }

    String iOSBuildSettings::getProjectName() const
    {
        return PathUtil::GetPureFilename( Engine::instance()->getConfig().m_projectFile, false);
    }

    String iOSBuildSettings::getAppName() const
    {
        if(m_appName.empty())   return getProjectName();
        else                    return m_appName;
    }

    String iOSBuildSettings::getIdentifier() const
    {
        if(m_identifier.empty())
        {
            String appName = PathUtil::GetPureFilename( Engine::instance()->getConfig().m_projectFile, false);
            String identifier = "com.echo." + appName;
            identifier = StringUtil::Replace( identifier, ' ', '_');
            
            return identifier;
        }
        else
        {
            return m_identifier;
        }
    }

    String iOSBuildSettings::getFinalResultPath()
    {
        String FinalResultPath = m_outputDir + "bin/app/";
        return PathUtil::IsDirExist(FinalResultPath) ? FinalResultPath : m_outputDir;
    }

    void iOSBuildSettings::writeUIInterfaceOrientationInfo(void* parent)
    {
        pugi::xml_node* root_dict = (pugi::xml_node*)(parent);
        
        root_dict->append_child("key").append_child(pugi::node_pcdata).set_value("UIRequiresFullScreen");
        root_dict->append_child("true");
        
        // https://developer.apple.com/documentation/uikit/uiinterfaceorientation?language=objc
        root_dict->append_child("key").append_child(pugi::node_pcdata).set_value("UISupportedInterfaceOrientations");
        pugi::xml_node orient_node = root_dict->append_child("array");
        
        if(m_uiInterfaceOrientationPortrait)
            orient_node.append_child("string").append_child(pugi::node_pcdata).set_value("UIInterfaceOrientationPortrait");
        
        if(m_uiInterfaceOrientationPortraitUpsideDown)
            orient_node.append_child("string").append_child(pugi::node_pcdata).set_value("UIInterfaceOrientationPortraitUpsideDown");
        
        if(m_uiInterfaceOrientationLandscapeLeft)
            orient_node.append_child("string").append_child(pugi::node_pcdata).set_value("UIInterfaceOrientationLandscapeLeft");
        
        if(m_uiInterfaceOrientationLandscapeRight)
            orient_node.append_child("string").append_child(pugi::node_pcdata).set_value("UIInterfaceOrientationLandscapeRight");
    }

    void iOSBuildSettings::writeLaunchImageInfo(void* parent)
    {
        pugi::xml_node* root_dict = (pugi::xml_node*)(parent);
        
        // iphone
        root_dict->append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImages~iphone");
        pugi::xml_node iphone_node = root_dict->append_child("array");
        
        // ipad
        root_dict->append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImages~ipad");
        pugi::xml_node ipad_node = root_dict->append_child("array");
        
        for(const LaunchImageItem& image : m_launchImages)
        {
            i32 minDimension = std::min<i32>(image.m_width, image.m_height);
            i32 maxDimension = std::max<i32>(image.m_width, image.m_height);
            pugi::xml_node dict_parent = image.m_type == DeviceType::iPhone ? iphone_node : ipad_node;
            
            if(m_uiInterfaceOrientationPortrait || m_uiInterfaceOrientationPortraitUpsideDown)
            {
                pugi::xml_node dict_node = dict_parent.append_child("dict");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageMinimumOSVersion");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value("8.0");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageName");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value(image.getPortraitName().c_str());
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageOrientation");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value("Portrait");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageSize");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value(StringUtil::Format("{%d, %d}", minDimension, maxDimension).c_str());
            }
            
            if(m_uiInterfaceOrientationLandscapeLeft || m_uiInterfaceOrientationLandscapeRight)
            {
                pugi::xml_node dict_node = dict_parent.append_child("dict");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageMinimumOSVersion");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value("8.0");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageName");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value(image.getLandscapeName().c_str());
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageOrientation");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value("Landscape");
                
                dict_node.append_child("key").append_child(pugi::node_pcdata).set_value("UILaunchImageSize");
                dict_node.append_child("string").append_child(pugi::node_pcdata).set_value(StringUtil::Format("{%d, %d}", maxDimension, minDimension).c_str());
            }
        }
    }

    void iOSBuildSettings::writeInfoPlist()
    {
        pugi::xml_document doc;
        pugi::xml_node dec = doc.prepend_child(pugi::node_declaration);
        dec.append_attribute("version") = "1.0";
        dec.append_attribute("encoding") = "utf-8";

        pugi::xml_node root_node= doc.append_child("plist" );
        root_node.append_attribute("version").set_value("1.0");
        
        pugi::xml_node root_dict = root_node.append_child("dict");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleVersion");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value(m_version.c_str());
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleShortVersionString");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value(m_version.c_str());
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleDevelopmentRegion");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("English");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleExecutable");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("$(EXECUTABLE_NAME)");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleGetInfoString");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleIconName");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("AppIcon");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleIdentifier");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value(getIdentifier().c_str());
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleInfoDictionaryVersion");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("6.0");
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundleName");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value(getAppName().c_str());
        
        root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("CFBundlePackageType");
        root_dict.append_child("string").append_child(pugi::node_pcdata).set_value("APPL");
        
        if(m_hiddenStatusBar)
        {
            root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("UIStatusBarHidden");
            root_dict.append_child("true");
            
            root_dict.append_child("key").append_child(pugi::node_pcdata).set_value("UIViewControllerBasedStatusBarAppearance");
            root_dict.append_child("false");
        }
        
        writeUIInterfaceOrientationInfo(&root_dict);
        writeLaunchImageInfo(&root_dict);

        Echo::String savePath = m_outputDir + "app/ios/frame/Platform/iOS/Info.plist";
        doc.save_file(savePath.c_str(), "\t", 1U, pugi::encoding_utf8);
    }

    void iOSBuildSettings::writeCMakeList()
    {
        String  cmakeStr;
        
        // module
        String moduleName = StringUtil::Replace(getProjectName(), " ", "");
        StringUtil::WriteLine( cmakeStr, StringUtil::Format("SET(MODULE_NAME %s)", moduleName.c_str()));
        
        // set module path
        StringUtil::WriteLine( cmakeStr, "SET(MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})");
        
        // include directories
        StringUtil::WriteLine( cmakeStr, "INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})");
        StringUtil::WriteLine( cmakeStr, "INCLUDE_DIRECtORIES(${ECHO_ROOT_PATH})");
        
        // link directories
        StringUtil::WriteLine( cmakeStr, "LINK_DIRECTORIES(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})");
        StringUtil::WriteLine( cmakeStr, "LINK_DIRECTORIES(${ECHO_LIB_PATH})");
        StringUtil::WriteLine( cmakeStr, "LINK_DIRECTORIES(${ECHO_ROOT_PATH}/thirdparty/live2d/Cubism31SdkNative-EAP5/Core/lib/ios/)");
        
        // AddFrameWork Macro
        StringUtil::WriteLine( cmakeStr, "MACRO(ADD_FRAMEWORK fwname)");
        StringUtil::WriteLine( cmakeStr, "    SET(FRAMEWORKS \"${FRAMEWORKS} -framework ${fwname}\")");
        StringUtil::WriteLine( cmakeStr, "ENDMACRO(ADD_FRAMEWORK)");
        
        // Get all project files recursively
        StringUtil::WriteLine( cmakeStr, "FILE(GLOB_RECURSE HEADER_FILES *.h *.inl)");
        StringUtil::WriteLine( cmakeStr, "FILE(GLOB_RECURSE SOURCE_FILES *.cpp *.m *.mm)");
        
        StringUtil::WriteLine( cmakeStr, "SET(ALL_FILES ${HEADER_FILES} ${SOURCE_FILES})");
        
        // group source files
        StringUtil::WriteLine( cmakeStr, "GROUP_FILES(ALL_FILES ${CMAKE_CURRENT_SOURCE_DIR})");
        
        // iOS platform resources
        StringUtil::WriteLine( cmakeStr, "SET(IOS_RESOURCE_FILES");
        
        // icon
        StringUtil::WriteLine( cmakeStr, "    ${MODULE_PATH}/Assets.xcassets");
        
        // launch images
        for(const LaunchImageItem& image : m_launchImages)
        {
            if(m_uiInterfaceOrientationPortrait || m_uiInterfaceOrientationPortraitUpsideDown)
                StringUtil::WriteLine( cmakeStr, image.getPortraitPath());
            
            if(m_uiInterfaceOrientationLandscapeLeft || m_uiInterfaceOrientationLandscapeRight)
                StringUtil::WriteLine(cmakeStr, image.getLandscapePath());
        }
        
        // data
        StringUtil::WriteLine( cmakeStr, "    ${MODULE_PATH}/resources/data");
        StringUtil::WriteLine( cmakeStr, ")");
        StringUtil::WriteLine( cmakeStr, "SET_SOURCE_FILES_PROPERTIES(${IOS_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)");
        
        // add framework
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(MediaPlayer)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(AudioToolbox)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreGraphics)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(QuartzCore)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(UIKit)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(GLKit)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(OpenGLES)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(OpenAL)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CFNetwork)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(MobileCoreServices)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(SystemConfiguration)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(AVFoundation)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(AdSupport)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreFoundation)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreTelephony)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreText)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(iAd)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(StoreKit)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreData)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreMedia)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(CoreMotion)");
        StringUtil::WriteLine( cmakeStr, "ADD_FRAMEWORK(Security)");
        
        // settings
        StringUtil::WriteLine( cmakeStr, "SET(FRAMEWORKS \"${FRAMEWORKS} -ObjC\")");
        StringUtil::WriteLine( cmakeStr, "SET(CMAKE_EXE_LINKER_FLAGS ${FRAMEWORKS})");
        StringUtil::WriteLine( cmakeStr, "SET(CMAKE_OSX_ARCHITECTURES \"${ARCHS_STANDARD}\")");
        
        StringUtil::WriteLine( cmakeStr, "ADD_EXECUTABLE(${MODULE_NAME} MACOSX_BUNDLE ${HEADER_FILES} ${SOURCE_FILES} ${IOS_RESOURCE_FILES} CMakeLists.txt)");
        
        // link libraries
        StringUtil::WriteLine( cmakeStr, "TARGET_LINK_LIBRARIES(${MODULE_NAME} engine)");
        StringUtil::WriteLine( cmakeStr, "TARGET_LINK_LIBRARIES(${MODULE_NAME} pugixml physx spine recast lua freeimage freetype zlib box2d)");
        StringUtil::WriteLine( cmakeStr, "TARGET_LINK_LIBRARIES(${MODULE_NAME} Live2DCubismCore)");
        StringUtil::WriteLine( cmakeStr, "TARGET_LINK_LIBRARIES(${MODULE_NAME} glslang spirv-cross)");
        
        // set target properties
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MODULE_PATH}/Frame/Platform/iOS/Info.plist)");
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD \"c++14\")");
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY \"1,2\")");
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME \"AppIcon\")");

		// for archive https://stackoverflow.com/questions/10715211/cannot-generate-ios-app-archive-in-xcode
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES XCODE_ATTRIBUTE_SKIP_INSTALL  \"No\")");
        StringUtil::WriteLine( cmakeStr, "SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES XCODE_ATTRIBUTE_INSTALL_PATH  \"$(LOCAL_APPS_DIR)\")");
        
        // messages
        StringUtil::WriteLine( cmakeStr, "MESSAGE(STATUS \"Configure iOS App success!\")");
        
        // write to file
        String savePath = m_outputDir + "app/ios/CMakeLists.txt";
        IO::instance()->saveStringToFile(savePath, cmakeStr);
    }

	void iOSBuildSettings::writeModuleConfig()
	{
		String  moduleSrc;

		// include
        StringUtil::WriteLine(moduleSrc, "#include <engine/core/main/module.h>\n");

		// namespace
        StringUtil::WriteLine(moduleSrc, "namespace Echo\n{");
        StringUtil::WriteLine(moduleSrc, "\tvoid registerModules()");
        StringUtil::WriteLine(moduleSrc, "\t{");
		vector<Module*>::type* allModules = Module::getAllModules();
		if (allModules)
		{
			for (Module* module : *allModules)
			{
				if (module->isEnable() && !module->isEditorOnly())
                    StringUtil::WriteLine(moduleSrc, StringUtil::Format("\t\tREGISTER_MODULE(%s)", module->getClassName().c_str()));
			}
		}

		// end namespace
        StringUtil::WriteLine(moduleSrc, "\t}\n}\n");

		// Write to file
		String savePath = m_outputDir + "app/ios/frame/Config/ModuleConfig.cpp";
        IO::instance()->saveStringToFile(savePath, moduleSrc);
	}
}
