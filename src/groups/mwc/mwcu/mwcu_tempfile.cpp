// Copyright 2021-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// mwcu_tempfile.cpp                                                  -*-C++-*-
#include <mwcu_tempfile.h>

#include <mwcscm_version.h>
// MWC
#include <mwcu_temputil.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mwcu {

// --------------
// class TempFile
// --------------

// CREATORS
TempFile::TempFile(bslma::Allocator* basicAllocator)
: d_path(basicAllocator)
{
    const bsl::string prefix = mwcu::TempUtil::tempDir();

    bdls::FilesystemUtil::FileDescriptor descriptor =
        bdls::FilesystemUtil::createTemporaryFile(&d_path, prefix);

    BSLS_ASSERT_OPT(descriptor != bdls::FilesystemUtil::k_INVALID_FD);

    int rc = bdls::FilesystemUtil::close(descriptor);
    BSLS_ASSERT_OPT(rc == 0);
}

TempFile::~TempFile()
{
    int rc = bdls::FilesystemUtil::remove(d_path, false);
    BSLS_ASSERT_OPT(rc == 0);
}

// ACCESSORS
const bsl::string& TempFile::path() const
{
    return d_path;
}

}  // close package namespace
}  // close enterprise namespace
