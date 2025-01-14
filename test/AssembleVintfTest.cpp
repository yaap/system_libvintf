/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AssembleVintfTest"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <gtest/gtest.h>

#include <aidl/metadata.h>
#include <vintf/AssembleVintf.h>
#include <vintf/parse_string.h>
#include "constants-private.h"
#include "test_constants.h"

using android::base::StringPrintf;

namespace android {
namespace vintf {

static bool In(const std::string& sub, const std::string& str) {
    return str.find(sub) != std::string::npos;
}
#define EXPECT_IN(sub, str) EXPECT_TRUE(In((sub), (str))) << (str);

class AssembleVintfTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        mInstance = AssembleVintf::newInstance();
        auto s = makeStream("");
        mOutputStream = s.get();
        mInstance->setOutputStream(std::move(s));
        s = makeStream("");
        mErrorStream = s.get();
        mInstance->setErrorStream(std::move(s));

        getInstance()->setFakeEnv("PRODUCT_ENFORCE_VINTF_MANIFEST", "true");
    }
    virtual void TearDown() override { mInstance = nullptr; }

    const std::unique_ptr<AssembleVintf>& getInstance() { return mInstance; }

    std::string getOutput() { return mOutputStream->str(); }
    std::string getError() { return mErrorStream->str(); }

    void resetOutput() { mOutputStream->str(""); }

    void setFakeEnvs(const std::map<std::string, std::string>& envs) {
        for (const auto& pair : envs) {
            getInstance()->setFakeEnv(pair.first, pair.second);
        }
    }

    void setFakeAidlMetadata(const std::vector<AidlInterfaceMetadata>& metadata) {
        getInstance()->setFakeAidlMetadata(metadata);
    }

    void setFakeAidlUseUnfrozen(bool use) { getInstance()->setFakeAidlUseUnfrozen(use); }

    void addInput(const std::string& name, const std::string& s) {
        getInstance()->addInputStream(name, makeStream(s));
    }

    std::unique_ptr<std::stringstream> makeStream(const std::string& s) {
        return std::make_unique<std::stringstream>(s);
    }

    std::unique_ptr<AssembleVintf> mInstance;
    // do not own this object.
    std::stringstream* mOutputStream;
    std::stringstream* mErrorStream;
};

// clang-format off

TEST_F(AssembleVintfTest, FrameworkMatrixEmpty) {
    std::string xmlEmpty = "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" />";
    std::string kernel318 = "CONFIG_FOO=y\n";
    std::string kernel318_64 = "CONFIG_BAR=y\n";
    std::string kernel44 = "# CONFIG_FOO is not set\n";
    std::string kernel44_64 = "CONFIG_BAR=y\n";

    addInput("compatibility_matrix.empty.xml", xmlEmpty);
    setFakeEnvs({
        {"POLICYVERS", "30"},
        {"PLATFORM_SEPOLICY_VERSION", "202404"},
        {"FRAMEWORK_VBMETA_VERSION", "1.0"},
    });
    getInstance()->addKernelConfigInputStream({3, 18, 0}, "android-base.config",
                                              makeStream(kernel318));
    getInstance()->addKernelConfigInputStream({3, 18, 0}, "android-base-arm64.config",
                                              makeStream(kernel318_64));
    getInstance()->addKernelConfigInputStream({4, 4, 0}, "android-base.config", makeStream(kernel44));
    getInstance()->addKernelConfigInputStream({4, 4, 0}, "android-base-arm64.config",
                                              makeStream(kernel44_64));

    EXPECT_TRUE(getInstance()->assemble());

    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
        "    <kernel version=\"3.18.0\">\n"
        "        <config>\n"
        "            <key>CONFIG_FOO</key>\n"
        "            <value type=\"tristate\">y</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "    <kernel version=\"3.18.0\">\n"
        "        <conditions>\n"
        "            <config>\n"
        "                <key>CONFIG_ARM64</key>\n"
        "                <value type=\"tristate\">y</value>\n"
        "            </config>\n"
        "        </conditions>\n"
        "        <config>\n"
        "            <key>CONFIG_BAR</key>\n"
        "            <value type=\"tristate\">y</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "    <kernel version=\"4.4.0\">\n"
        "        <config>\n"
        "            <key>CONFIG_FOO</key>\n"
        "            <value type=\"tristate\">n</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "    <kernel version=\"4.4.0\">\n"
        "        <conditions>\n"
        "            <config>\n"
        "                <key>CONFIG_ARM64</key>\n"
        "                <value type=\"tristate\">y</value>\n"
        "            </config>\n"
        "        </conditions>\n"
        "        <config>\n"
        "            <key>CONFIG_BAR</key>\n"
        "            <value type=\"tristate\">y</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "    <sepolicy>\n"
        "        <kernel-sepolicy-version>30</kernel-sepolicy-version>\n"
        "        <sepolicy-version>202404</sepolicy-version>\n"
        "    </sepolicy>\n"
        "    <avb>\n"
        "        <vbmeta-version>1.0</vbmeta-version>\n"
        "    </avb>\n"
        "</compatibility-matrix>\n",
        getOutput());
}

TEST_F(AssembleVintfTest, FrameworkMatrix) {
    std::string tail =
        "        <config>\n"
        "            <key>CONFIG_FOO</key>\n"
        "            <value type=\"tristate\">y</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "    <sepolicy>\n"
        "        <kernel-sepolicy-version>30</kernel-sepolicy-version>\n"
        "        <sepolicy-version>202404</sepolicy-version>\n"
        "    </sepolicy>\n"
        "    <avb>\n"
        "        <vbmeta-version>1.0</vbmeta-version>\n"
        "    </avb>\n"
        "</compatibility-matrix>\n";

    std::string xmlEmpty =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
        "    <kernel version=\"3.18.0\">\n" +
        tail;

    std::string xml1 =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"1\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</compatibility-matrix>\n";

    std::string xml2 =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"2\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>1.0-1</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</compatibility-matrix>\n";

    std::string xml3 =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"3\">\n"
        "    <hal format=\"hidl\" optional=\"false\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</compatibility-matrix>\n";

    auto manifest = [](size_t level) {
        return "<manifest " +
                    kMetaVersionStr +
                    " type=\"device\"" +
                    " target-level=\"" + std::to_string(level) + "\">\n" +
               "    <hal format=\"hidl\">\n"
               "        <name>android.hardware.foo</name>\n"
               "        <version>1.1</version>\n"
               "        <transport>hwbinder</transport>\n"
               "        <interface>\n"
               "            <name>IFoo</name>\n"
               "            <instance>default</instance>\n"
               "        </interface>\n"
               "    </hal>\n"
               "    <hal format=\"hidl\">\n"
               "        <name>android.hardware.foo</name>\n"
               "        <version>2.0</version>\n"
               "        <transport>hwbinder</transport>\n"
               "        <interface>\n"
               "            <name>IFoo</name>\n"
               "            <instance>default</instance>\n"
               "        </interface>\n"
               "    </hal>\n"
               "    <sepolicy>\n"
               "        <version>202404</version>\n"
               "    </sepolicy>\n"
               "</manifest>\n";
    };

    addInput("compatibility_matrix.1.xml", xml1);
    addInput("compatibility_matrix.2.xml", xml2);
    addInput("compatibility_matrix.3.xml", xml3);
    addInput("compatibility_matrix.empty.xml", xmlEmpty);
    getInstance()->setFakeEnv("PRODUCT_ENFORCE_VINTF_MANIFEST", "true");

    resetOutput();
    getInstance()->setCheckInputStream("check.xml", makeStream(manifest(1)));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"1\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>1.0-1</version>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <kernel version=\"3.18.0\" level=\"1\">\n" +
            tail,
        getOutput());

    resetOutput();
    getInstance()->setCheckInputStream("check.xml", makeStream(manifest(2)));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"2\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>1.0-1</version>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <kernel version=\"3.18.0\" level=\"2\">\n" +
            tail,
        getOutput());

    resetOutput();
    getInstance()->setCheckInputStream("check.xml", makeStream(manifest(3)));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"3\">\n"
        "    <hal format=\"hidl\" optional=\"false\">\n"
        "        <name>android.hardware.foo</name>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <kernel version=\"3.18.0\" level=\"3\">\n" +
            tail,
        getOutput());
}

TEST_F(AssembleVintfTest, MatrixVendorNdk) {
    addInput("compatibility_matrix.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"device\"/>\n");
    getInstance()->setFakeEnv("REQUIRED_VNDK_VERSION", "P");
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"device\">\n"
        "    <vendor-ndk>\n"
        "        <version>P</version>\n"
        "    </vendor-ndk>\n"
        "</compatibility-matrix>\n",
        getOutput());
}

TEST_F(AssembleVintfTest, ManifestVendorNdk) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + " type=\"framework\"/>\n");
    getInstance()->setFakeEnv("PROVIDED_VNDK_VERSIONS", "P  26 27   ");
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<manifest " + kMetaVersionStr + " type=\"framework\">\n"
        "    <vendor-ndk>\n"
        "        <version>P</version>\n"
        "    </vendor-ndk>\n"
        "    <vendor-ndk>\n"
        "        <version>26</version>\n"
        "    </vendor-ndk>\n"
        "    <vendor-ndk>\n"
        "        <version>27</version>\n"
        "    </vendor-ndk>\n"
        "</manifest>\n",
        getOutput());
}

TEST_F(AssembleVintfTest, VendorNdkCheckEmpty) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + " type=\"framework\"/>\n");
    getInstance()->setFakeEnv("PROVIDED_VNDK_VERSIONS", "P 26 27 ");

    std::string matrix = "<compatibility-matrix " + kMetaVersionStr + " type=\"device\"/>\n";
    getInstance()->setCheckInputStream("check.xml", makeStream(matrix));
    EXPECT_TRUE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, VendorNdkCheckIncompat) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + " type=\"framework\"/>\n");
    getInstance()->setFakeEnv("PROVIDED_VNDK_VERSIONS", "P 26 27 ");
    std::string matrix =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"device\">\n"
        "    <vendor-ndk>\n"
        "        <version>O</version>\n"
        "    </vendor-ndk>\n"
        "</compatibility-matrix>\n";
    getInstance()->setCheckInputStream("check.xml", makeStream(matrix));
    EXPECT_FALSE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, VendorNdkCheckCompat) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + " type=\"framework\"/>\n");
    getInstance()->setFakeEnv("PROVIDED_VNDK_VERSIONS", "P 26 27 ");
    std::string matrix =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"device\">\n"
        "    <vendor-ndk>\n"
        "        <version>27</version>\n"
        "    </vendor-ndk>\n"
        "</compatibility-matrix>\n";
    getInstance()->setCheckInputStream("check.xml", makeStream(matrix));
    EXPECT_TRUE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, MatrixSystemSdk) {
    addInput("compatibility_matrix.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"device\"/>\n");
    getInstance()->setFakeEnv("BOARD_SYSTEMSDK_VERSIONS", "P 1 2 ");
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"device\">\n"
        "    <system-sdk>\n"
        "        <version>1</version>\n"
        "        <version>2</version>\n"
        "        <version>P</version>\n"
        "    </system-sdk>\n"
        "</compatibility-matrix>\n",
        getOutput());
}

TEST_F(AssembleVintfTest, ManifestSystemSdk) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + " type=\"framework\"/>\n");
    getInstance()->setFakeEnv("PLATFORM_SYSTEMSDK_VERSIONS", "P 1 2 ");
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<manifest " + kMetaVersionStr + " type=\"framework\">\n"
        "    <system-sdk>\n"
        "        <version>1</version>\n"
        "        <version>2</version>\n"
        "        <version>P</version>\n"
        "    </system-sdk>\n"
        "</manifest>\n",
        getOutput());
}

const std::string gEmptyOutManifest =
    "<manifest " + kMetaVersionStr + " type=\"device\">\n"
    "    <sepolicy>\n"
    "        <version>202404</version>\n"
    "    </sepolicy>\n"
    "</manifest>\n";

TEST_F(AssembleVintfTest, EmptyManifest) {
    const std::string emptyManifest = "<manifest " + kMetaVersionStr + " type=\"device\" />";
    setFakeEnvs({{"BOARD_SEPOLICY_VERS", "202404"}, {"IGNORE_TARGET_FCM_VERSION", "true"}});
    addInput("manifest.empty.xml", emptyManifest);
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(gEmptyOutManifest, getOutput());
}

TEST_F(AssembleVintfTest, DeviceFrameworkMatrixOptional) {
    setFakeEnvs({{"POLICYVERS", "30"},
                 {"PLATFORM_SEPOLICY_VERSION", "202404"},
                 {"PLATFORM_SEPOLICY_COMPAT_VERSIONS", "26.0 27.0"},
                 {"FRAMEWORK_VBMETA_VERSION", "1.0"},
                 {"PRODUCT_ENFORCE_VINTF_MANIFEST", "true"}});
    getInstance()->setCheckInputStream("check.xml", makeStream(gEmptyOutManifest));

    addInput("compatibility_matrix.empty.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
             "    <hal format=\"hidl\" optional=\"true\">\n"
             "        <name>vendor.foo.bar</name>\n"
             "        <version>1.0</version>\n"
             "        <interface>\n"
             "            <name>IFoo</name>\n"
             "            <instance>default</instance>\n"
             "        </interface>\n"
             "    </hal>\n"
             "</compatibility-matrix>");

    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>vendor.foo.bar</name>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <sepolicy>\n"
        "        <kernel-sepolicy-version>30</kernel-sepolicy-version>\n"
        "        <sepolicy-version>26.0</sepolicy-version>\n"
        "        <sepolicy-version>27.0</sepolicy-version>\n"
        "        <sepolicy-version>202404</sepolicy-version>\n"
        "    </sepolicy>\n"
        "    <avb>\n"
        "        <vbmeta-version>1.0</vbmeta-version>\n"
        "    </avb>\n"
        "</compatibility-matrix>",
        getOutput());
}

TEST_F(AssembleVintfTest, DeviceFrameworkMatrixRequired) {
    setFakeEnvs({{"POLICYVERS", "30"},
                 {"PLATFORM_SEPOLICY_VERSION", "202404"},
                 {"PLATFORM_SEPOLICY_COMPAT_VERSIONS", "26.0 27.0"},
                 {"FRAMEWORK_VBMETA_VERSION", "1.0"},
                 {"PRODUCT_ENFORCE_VINTF_MANIFEST", "true"}});
    getInstance()->setCheckInputStream("check.xml", makeStream(gEmptyOutManifest));

    addInput("compatibility_matrix.empty.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
             "    <hal format=\"hidl\" optional=\"false\">\n"
             "        <name>vendor.foo.bar</name>\n"
             "        <version>1.0</version>\n"
             "        <interface>\n"
             "            <name>IFoo</name>\n"
             "            <instance>default</instance>\n"
             "        </interface>\n"
             "    </hal>\n"
             "</compatibility-matrix>");

    EXPECT_FALSE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, DeviceFrameworkMatrixMultiple) {
    setFakeEnvs({{"POLICYVERS", "30"},
                 {"PLATFORM_SEPOLICY_VERSION", "202404"},
                 {"PLATFORM_SEPOLICY_COMPAT_VERSIONS", "26.0 27.0"},
                 {"FRAMEWORK_VBMETA_VERSION", "1.0"},
                 {"PRODUCT_ENFORCE_VINTF_MANIFEST", "true"}});
    getInstance()->setCheckInputStream("check.xml", makeStream(gEmptyOutManifest));

    addInput("compatibility_matrix.foobar.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
             "    <hal format=\"hidl\" optional=\"true\">\n"
             "        <name>vendor.foo.bar</name>\n"
             "        <version>1.0</version>\n"
             "        <interface>\n"
             "            <name>IFoo</name>\n"
             "            <instance>default</instance>\n"
             "        </interface>\n"
             "    </hal>\n"
             "</compatibility-matrix>");

    addInput("compatibility_matrix.bazquux.xml",
             "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
             "    <hal format=\"hidl\" optional=\"true\">\n"
             "        <name>vendor.baz.quux</name>\n"
             "        <version>1.0</version>\n"
             "        <interface>\n"
             "            <name>IBaz</name>\n"
             "            <instance>default</instance>\n"
             "        </interface>\n"
             "    </hal>\n"
             "</compatibility-matrix>");

    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\">\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>vendor.baz.quux</name>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IBaz</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\" optional=\"true\">\n"
        "        <name>vendor.foo.bar</name>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IFoo</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <sepolicy>\n"
        "        <kernel-sepolicy-version>30</kernel-sepolicy-version>\n"
        "        <sepolicy-version>26.0</sepolicy-version>\n"
        "        <sepolicy-version>27.0</sepolicy-version>\n"
        "        <sepolicy-version>202404</sepolicy-version>\n"
        "    </sepolicy>\n"
        "    <avb>\n"
        "        <vbmeta-version>1.0</vbmeta-version>\n"
        "    </avb>\n"
        "</compatibility-matrix>",
        getOutput());
}

TEST_F(AssembleVintfTest, OutputFileMatrixTest) {
    const std::string kFile = "file_name_1.xml";
    const std::string kMatrix =
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\"/>";
    addInput(kFile, kMatrix);
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(kFile, getOutput());
}

TEST_F(AssembleVintfTest, OutputFileManifestTest) {
    const std::string kFile = "file_name_1.xml";
    std::string kManifest = "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\"/>";
    addInput(kFile, kManifest);
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(kFile, getOutput());
}

TEST_F(AssembleVintfTest, AidlAndHidlNames) {
    addInput("manifest1.xml",
        "<manifest " + kMetaVersionStr + " type=\"framework\">\n"
        "    <hal format=\"aidl\">\n"
        "        <name>android.system.foo</name>\n"
        "        <fqname>IFoo/default</fqname>\n"
        "    </hal>\n"
        "</manifest>\n");
    addInput("manifest2.xml",
        "<manifest " + kMetaVersionStr + " type=\"framework\">\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.system.foo</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <fqname>@1.0::IFoo/default</fqname>\n"
        "    </hal>\n"
        "</manifest>\n");
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "android.system.foo",
         .types = {"android.system.foo.IFoo"}}};
    setFakeAidlMetadata(aidl);
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "    <hal format=\"aidl\">\n"
        "        <name>android.system.foo</name>\n"
        "        <fqname>IFoo/default</fqname>\n"
        "    </hal>\n",
        getOutput());
    EXPECT_IN(
        "    <hal format=\"hidl\">\n"
        "        <name>android.system.foo</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <fqname>@1.0::IFoo/default</fqname>\n"
        "    </hal>\n",
        getOutput());
}

// Merge kernel FCM from manually written device manifest and <config> from
// parsing kernel prebuilt.
TEST_F(AssembleVintfTest, MergeKernelFcmAndConfigs) {
    addInput("manifest.xml",
        "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\">\n"
        "    <kernel target-level=\"2\"/>\n"
        "</manifest>\n");
    getInstance()->addKernelConfigInputStream({3, 18, 10}, "android-base.config",
                                              makeStream("CONFIG_FOO=y"));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<kernel version=\"3.18.10\" target-level=\"2\">", getOutput());
}

TEST_F(AssembleVintfTest, NoAutoSetKernelFcm) {
    addInput("manifest.xml",
        "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\">\n"
        "    <kernel version=\"3.18.10\"/>\n"
        "</manifest>\n");
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<kernel version=\"3.18.10\"/>", getOutput());
}

TEST_F(AssembleVintfTest, NoAutoSetKernelFcmWithConfig) {
    addInput("manifest.xml",
        "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\" />\n");
    getInstance()->addKernelConfigInputStream({3, 18, 10}, "android-base.config",
                                              makeStream("CONFIG_FOO=y"));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<kernel version=\"3.18.10\">", getOutput());
}

TEST_F(AssembleVintfTest, NoKernelFcmT) {
    addInput("manifest.xml",
        StringPrintf(R"(<manifest %s type="device" target-level="%s">
                            <kernel target-level="8"/>
                        </manifest>)", kMetaVersionStr.c_str(),
                        to_string(details::kEnforceDeviceManifestNoKernelLevel).c_str()));
    EXPECT_FALSE(getInstance()->assemble());
}

// Automatically add kernel FCM when parsing framework matrix for a single FCM version.
TEST_F(AssembleVintfTest, AutoSetMatrixKernelFcm) {
    addInput("compatibility_matrix.xml",
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"1\"/>\n"
    );
    getInstance()->addKernelConfigInputStream({3, 18, 10}, "android-base.config",
                                              makeStream(""));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<kernel version=\"3.18.10\" level=\"1\"/>", getOutput());
}


TEST_F(AssembleVintfTest, WithKernelRequirements) {
    setFakeEnvs({{"POLICYVERS", "30"},
                 {"PLATFORM_SEPOLICY_VERSION", "202404"},
                 {"PRODUCT_ENFORCE_VINTF_MANIFEST", "true"}});
    addInput("compatibility_matrix.xml",
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"1\">\n"
        "    <kernel version=\"3.18.1\" level=\"1\">\n"
        "        <config>\n"
        "            <key>CONFIG_FOO</key>\n"
        "            <value type=\"tristate\">y</value>\n"
        "        </config>\n"
        "    </kernel>\n"
        "</compatibility-matrix>\n");
    getInstance()->setCheckInputStream("check.xml", makeStream(
        "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\">\n"
        "    <kernel target-level=\"1\" version=\"3.18.0\"/>\n"
        "    <sepolicy>\n"
        "        <version>202404</version>\n"
        "    </sepolicy>\n"
        "</manifest>\n"));

    EXPECT_FALSE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, NoKernelRequirements) {
    setFakeEnvs({{"POLICYVERS", "30"},
                 {"PLATFORM_SEPOLICY_VERSION", "202404"},
                 {"PRODUCT_ENFORCE_VINTF_MANIFEST", "true"}});
    addInput("compatibility_matrix.xml",
        "<compatibility-matrix " + kMetaVersionStr + " type=\"framework\" level=\"1\">\n"
        "    <kernel version=\"3.18.0\" level=\"1\"/>\n"
        "</compatibility-matrix>\n");
    getInstance()->setCheckInputStream("check.xml", makeStream(
        "<manifest " + kMetaVersionStr + " type=\"device\" target-level=\"1\">\n"
        "    <kernel target-level=\"1\"/>\n"
        "    <sepolicy>\n"
        "        <version>202404</version>\n"
        "    </sepolicy>\n"
        "</manifest>\n"));

    EXPECT_TRUE(getInstance()->setNoKernelRequirements());
    EXPECT_TRUE(getInstance()->assemble());
}

// clang-format on

TEST_F(AssembleVintfTest, ManifestLevelConflictCorrectLocation) {
    addInput("manifest.xml", "<manifest " + kMetaVersionStr + R"( type="device" />)");
    addInput("manifest_1.xml",
             "<manifest " + kMetaVersionStr + R"( type="device" target-level="1" />)");
    addInput("manifest_2.xml",
             "<manifest " + kMetaVersionStr + R"( type="device" target-level="2" />)");
    EXPECT_FALSE(getInstance()->assemble());
    EXPECT_IN("File 'manifest_1.xml' has level 1", getError());
    EXPECT_IN("File 'manifest_2.xml' has level 2", getError());
}

TEST_F(AssembleVintfTest, PassMultipleManifestEntrySameModule) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "android.system.foo",
         .stability = "vintf",
         .versions = {1, 2},
         .types = {"android.system.foobar.IFoo", "android.system.foobar.IBar"}}};
    setFakeAidlMetadata(aidl);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.foobar</name>\n"
                        <fqname>IFoo/default</fqname>\n"
                        <fqname>IBar/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    EXPECT_TRUE(getInstance()->assemble());
}

TEST_F(AssembleVintfTest, FailOnMultipleModulesInSameManifestEntry) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    std::vector<AidlInterfaceMetadata> aidl{{.name = "android.system.foo",
                                             .stability = "vintf",
                                             .versions = {1, 2},
                                             .types = {"android.system.foobar.IFoo"}},
                                            {.name = "android.system.bar",
                                             .stability = "vintf",
                                             .versions = {1, 2},
                                             .types = {"android.system.foobar.IBar"}}};
    setFakeAidlMetadata(aidl);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.foobar</name>\n"
                        <fqname>IFoo/default</fqname>\n"
                        <fqname>IBar/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    EXPECT_FALSE(getInstance()->assemble());
    EXPECT_IN("HAL manifest entries must only contain", getError());
    EXPECT_IN("android.system.foobar.IFoo is in android.system.foo", getError());
}

TEST_F(AssembleVintfTest, ForceDowngradeVersion) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "foo_android.system.bar",
         .stability = "vintf",
         .types = {"android.system.bar.IFoo", "android.system.bar.MyFoo"},
         .versions = {1, 2},
         .has_development = true}};
    setFakeAidlMetadata(aidl);
    setFakeAidlUseUnfrozen(false);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.bar</name>\n"
                        <fqname>IFoo/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<version>2</version>", getOutput());
}

TEST_F(AssembleVintfTest, InfoDowngradeVersionTypo) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "foo_android.system.bar",
         .stability = "vintf",
         .types = {"android.system.bar.IFoo", "android.system.bar.MyFoo"},
         .versions = {1, 2},
         .has_development = true}};
    setFakeAidlMetadata(aidl);
    setFakeAidlUseUnfrozen(false);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.bar</name>\n"
                        <fqname>IFooooooooo/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    // It doesn't fail because there may be prebuilts, but make sure we do log it.
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN(
        "INFO: Couldn't find AIDL metadata for: android.system.bar.IFooooooooo in file "
        "manifest1.xml. "
        "Check "
        "spelling?",
        getError());
}

TEST_F(AssembleVintfTest, AllowUnfrozenVersion) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "foo_android.system.bar",
         .stability = "vintf",
         .types = {"android.system.bar.IFoo", "android.system.bar.MyFoo"},
         .versions = {1, 2},
         .has_development = true}};
    setFakeAidlMetadata(aidl);
    setFakeAidlUseUnfrozen(true);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.bar</name>\n"
                        <fqname>IFoo/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<version>3</version>", getOutput());
}

TEST_F(AssembleVintfTest, KeepFrozenVersion) {
    setFakeEnvs({{"VINTF_IGNORE_TARGET_FCM_VERSION", "true"}});
    // V3 is already frozen
    std::vector<AidlInterfaceMetadata> aidl{
        {.name = "foo_android.system.bar",
         .stability = "vintf",
         .types = {"android.system.bar.IFoo", "android.system.bar.MyFoo"},
         .versions = {1, 2, 3},
         .has_development = true}};
    setFakeAidlMetadata(aidl);
    setFakeAidlUseUnfrozen(false);
    addInput("manifest1.xml", StringPrintf(
                                  R"(
                <manifest %s type="framework">
                   <hal format="aidl">
                        <name>android.system.bar</name>\n"
                        <fqname>IFoo/default</fqname>\n"
                        <version>3</version>\n"
                    </hal>
                </manifest>)",
                                  kMetaVersionStr.c_str()));
    EXPECT_TRUE(getInstance()->assemble());
    EXPECT_IN("<version>3</version>", getOutput());
}

}  // namespace vintf
}  // namespace android
