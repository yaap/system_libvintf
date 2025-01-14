/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef LIBVINTF_TARGET
#define LOG_TAG "libvintf"
#include <android-base/logging.h>
#endif

#include "ManifestInstance.h"

#include <utility>

#include <android-base/logging.h>

#include "parse_string.h"

namespace android {
namespace vintf {

ManifestInstance::ManifestInstance() = default;

ManifestInstance::ManifestInstance(const ManifestInstance&) = default;

ManifestInstance::ManifestInstance(ManifestInstance&&) noexcept = default;

ManifestInstance& ManifestInstance::operator=(const ManifestInstance&) = default;

ManifestInstance& ManifestInstance::operator=(ManifestInstance&&) noexcept = default;

ManifestInstance::ManifestInstance(FqInstance&& fqInstance, TransportArch&& ta, HalFormat fmt,
                                   std::optional<std::string>&& updatableViaApex,
                                   std::optional<std::string>&& accessor, bool updatableViaSystem)
    : mFqInstance(std::move(fqInstance)),
      mTransportArch(std::move(ta)),
      mHalFormat(fmt),
      mUpdatableViaApex(std::move(updatableViaApex)),
      mAccessor(std::move(accessor)),
      mUpdatableViaSystem(std::move(updatableViaSystem)) {}

ManifestInstance::ManifestInstance(const FqInstance& fqInstance, const TransportArch& ta,
                                   HalFormat fmt,
                                   const std::optional<std::string>& updatableViaApex,
                                   const std::optional<std::string>& accessor,
                                   bool updatableViaSystem)
    : mFqInstance(fqInstance),
      mTransportArch(ta),
      mHalFormat(fmt),
      mUpdatableViaApex(updatableViaApex),
      mAccessor(accessor),
      mUpdatableViaSystem(updatableViaSystem) {}

const std::string& ManifestInstance::package() const {
    return mFqInstance.getPackage();
}

Version ManifestInstance::version() const {
    return mFqInstance.getVersion();
}

std::string ManifestInstance::interface() const {
    return mFqInstance.getInterface();
}

const std::string& ManifestInstance::instance() const {
    return mFqInstance.getInstance();
}

Transport ManifestInstance::transport() const {
    return mTransportArch.transport;
}

Arch ManifestInstance::arch() const {
    return mTransportArch.arch;
}

const std::optional<std::string> ManifestInstance::ip() const {
    return mTransportArch.ip;
}

const std::optional<uint64_t> ManifestInstance::port() const {
    return mTransportArch.port;
}

HalFormat ManifestInstance::format() const {
    return mHalFormat;
}

const std::optional<std::string>& ManifestInstance::updatableViaApex() const {
    return mUpdatableViaApex;
}

const std::optional<std::string>& ManifestInstance::accessor() const {
    return mAccessor;
}

const FqInstance& ManifestInstance::getFqInstance() const {
    return mFqInstance;
}

bool ManifestInstance::updatableViaSystem() const {
    return mUpdatableViaSystem;
}

bool ManifestInstance::operator==(const ManifestInstance& other) const {
    return mFqInstance == other.mFqInstance && mTransportArch == other.mTransportArch &&
           mHalFormat == other.mHalFormat && mUpdatableViaApex == other.mUpdatableViaApex &&
           mUpdatableViaSystem == other.mUpdatableViaSystem && mAccessor == other.mAccessor;
}
bool ManifestInstance::operator<(const ManifestInstance& other) const {
    if (mFqInstance < other.mFqInstance) return true;
    if (other.mFqInstance < mFqInstance) return false;
    if (mTransportArch < other.mTransportArch) return true;
    if (other.mTransportArch < mTransportArch) return false;
    if (mHalFormat < other.mHalFormat) return true;
    if (other.mHalFormat < mHalFormat) return false;
    if (mUpdatableViaApex < other.mUpdatableViaApex) return true;
    if (other.mUpdatableViaApex < mUpdatableViaApex) return false;
    if (mUpdatableViaSystem < other.mUpdatableViaSystem) return true;
    if (other.mUpdatableViaSystem < mUpdatableViaSystem) return false;
    return mAccessor < other.mAccessor;
}

std::string ManifestInstance::getSimpleFqInstance() const {
    FqInstance e;
    bool success = false;
    switch (format()) {
        case HalFormat::AIDL: {
            // Hide fake version when printing to manifest XML <fqname> tag.
            success = e.setTo(interface(), instance());
        } break;
        case HalFormat::HIDL:
            [[fallthrough]];
        case HalFormat::NATIVE: {
            success = e.setTo(version().majorVer, version().minorVer, interface(), instance());
        } break;
    }
#ifndef LIBVINTF_TARGET
    CHECK(success) << "Cannot get simple fqinstnance from '" << mFqInstance.string() << "'";
#endif
    return success ? e.string() : "";
}

std::string ManifestInstance::description() const {
    switch (format()) {
        case HalFormat::AIDL: {
            return toAidlFqnameString(package(), interface(), instance()) + " (@" +
                   aidlVersionToString(version()) + ")";
        } break;
        case HalFormat::HIDL:
            [[fallthrough]];
        case HalFormat::NATIVE: {
            return getFqInstance().string();
        } break;
    }
}

std::string ManifestInstance::descriptionWithoutPackage() const {
    switch (format()) {
        case HalFormat::AIDL: {
            return toFQNameString(interface(), instance()) + " (@" +
                   aidlVersionToString(version()) + ")";
        } break;
        case HalFormat::HIDL:
            [[fallthrough]];
        case HalFormat::NATIVE: {
            return getSimpleFqInstance();
        } break;
    }
}

ManifestInstance ManifestInstance::withVersion(const Version& v) const {
    FqInstance fqInstance;
    CHECK(fqInstance.setTo(getFqInstance().getPackage(), v.majorVer, v.minorVer,
                           getFqInstance().getInterface(), getFqInstance().getInstance()));
    return ManifestInstance(std::move(fqInstance), mTransportArch, format(), mUpdatableViaApex,
                            mAccessor, mUpdatableViaSystem);
}

}  // namespace vintf
}  // namespace android
