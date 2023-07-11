// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqp_putmessageiterator.h                                          -*-C++-*-
#ifndef INCLUDED_BMQP_PUTMESSAGEITERATOR
#define INCLUDED_BMQP_PUTMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over messages of a 'PUT' event.
//
//@CLASSES:
//  bmqp::PutMessageIterator: read-only sequential iterator on 'PutEvent'.
//
//@DESCRIPTION: 'bmqp::PutMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a PutEvent.
//
/// Error handling: Logging and Assertion
///-------------------------------------
//: o logging: This iterator will not log anything in case of invalid data:
//:   this is the caller's responsibility to check the return value of
//:   'isValid()' and/or 'next()' and take action (the 'dumpBlob()' method can
//:   be used to print some helpful information).
//: o assertion: When built in SAFE mode, the iterator will assert when
//:   inconsistencies between the blob and the headers are detected.
//
/// Usage
///-----
// Typical usage of this iterator should follow the following pattern:
//..
//  int rc = 0;
//  while ((rc = putMessageIterator.next()) == 1) {
//    // Use putMessageIterator accessors, such as ..
//    int appDataSize = putMessageIterator.applicationDataSize();
//  }
//  if (rc < 0) {
//    // Invalid PutMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'PutEvent' [rc: " << rc << "]\n";
//        putMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//    }
//  }
//..

// BMQ

#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_blobiterator.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iterator.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
class MessageProperties;

// ========================
// class PutMessageIterator
// ========================

/// An iterator providing read-only sequential access to messages contained
/// into a `PutEvent`.
class PutMessageIterator {
  private:
    // PRIVATE TYPES

    typedef bdlb::NullableValue<OptionsView> NullableOptionsView;

  private:
    // DATA
    mwcu::BlobIterator d_blobIter;
    // Blob iterator pointing to the current
    // message in the blob.

    PutHeader d_header;
    // Deep copy of current PutHeader.
    // Force decompression (controlled by
    // 'd_isDecompressingOldMPs') results in
    // de-compressed data (in
    // 'd_applicationData') and the
    // corresponding 'header()' should not
    // have the compression flag while the
    // original blob and the original header
    // stay intact.  Therefore cannot use
    // BlobObjectProxy which could simply
    // alias to an offset in BlobBuffer and
    // rewrite it.

    mutable int d_applicationDataSize;
    // Computed application data real size
    // (without padding). -1 if not
    // initialized. Note that if
    // 'd_decompressFlag' is true, this will
    // store the size of decompressed
    // application data and vice-versa.

    mutable int d_lazyMessagePayloadSize;
    // Lazily computed payload real size
    // (without padding).  -1 if not
    // initialized.

    mutable mwcu::BlobPosition d_lazyMessagePayloadPosition;
    // Lazily computed payload position.
    // Unset if not initialized.

    mutable int d_messagePropertiesSize;
    // Message properties size.  0 if not
    // initialized.  Note that this length
    // includes padding and message
    // properties header.

    mutable mwcu::BlobPosition d_applicationDataPosition;
    // Application Data position. For each
    // blob, initialized in next().

    mutable int d_optionsSize;
    // Message options size.

    mutable mwcu::BlobPosition d_optionsPosition;
    // Message options position.  Unset if
    // not initialized.

    int d_advanceLength;
    // How much should we advance in the
    // blob when calling 'next()'.  The
    // iterator is considered in invalid
    // state if this value is == -1.

    mutable NullableOptionsView d_optionsView;
    // The OptionsView for this iterator.

    bool d_decompressFlag;
    // Flag indicating if message should be
    // decompressed when calling next().
    // false if not initialized.

    bdlbb::Blob d_applicationData;
    // Decompressed application data.
    // Populated only if d_decompressFlag is
    // true (empty otherwise).

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Buffer factory used for decompressed
    // application data.

    bool d_isDecompressingOldMPs;
    // Temporary; shall remove after 2ns
    // roll out of "new style" brokers.
    //
    // Recognize the following scenarios
    // 1) De-compress everything
    //      (d_decompressFlag == true)
    // 2) De-compress old format only
    //    (d_isDecompressingOldMPs == true)
    // 3) Do not de-compress
    //      (d_decompressFlag == false &&
    //      isDecompressingOldMPs == false)
    // Payload is de-compressed when
    // (d_decompressFlag == true ||
    //  (isDecompressingOldMPs == true &&
    //   isOldFormat == true))

    bslma::Allocator* d_allocator_p;
    // Allocator to use by this object.

  private:
    // PRIVATE MANIPULATORS

    /// Make this instance a copy of the specified `src`, that is copy and
    /// adjust each of its members to represent the same object as the one
    /// from `src`.
    void copyFrom(const PutMessageIterator& src);

    // PRIVATE ACCESSORS

    /// Load into `d_optionsView` a view over the options associated with
    /// the  message currently pointed to by this iterator.  Behavior is
    /// undefined unless latest call to `next()` returned 1.
    void initCachedOptionsView() const;

    /// Load into the specified `position` the position of payload for the
    /// message currently pointed to by this iterator.  Return zero on
    /// success, and a non-zero value otherwise.  Behavior is undefined
    /// unless `d_decompressFlag` is true and the latest call to `next()`
    /// returned 1.
    int loadMessagePayloadPosition(mwcu::BlobPosition* position) const;

    /// Return the size (in bytes) of compressed application data for the
    /// message currently pointed to by this iterator.  Behavior is
    /// undefined unless latest call to `next()` returned 1.  Note that
    /// compressed application data includes compressed message properties
    /// and message payload excluding message padding, and excludes options.
    int compressedApplicationDataSize() const;

  public:
    // CREATORS

    /// Create an invalid instance using the specified `allocator` and
    /// the specified `blobBufferFactory`.  The only valid operations on an
    /// invalid instance are assignment, `reset` and `isValid`.  If the
    /// optionally specified `isDecompressingOldMPs` is `true' and PUT
    /// message has compressed MessageProperties (old style), de-compress
    /// message.  Temporary, shall remove the 'isDecompressingMPs` argument
    /// the after all Brokers can read new style of compression.
    PutMessageIterator(bdlbb::BlobBufferFactory* bufferFactory,
                       bslma::Allocator*         allocator,
                       bool isDecompressingOldMPs = false);

    /// Initialize a new instance using the specified `blob`, `eventHeader`,
    /// `decompressFlag`, `blobBufferFactory` and `allocator`.  Behavior is
    /// undefined if the `blob` pointer is null, or the pointed-to blob does
    /// not contain enough bytes to fit at least the `eventHeader`.
    PutMessageIterator(const bdlbb::Blob*        blob,
                       const EventHeader&        eventHeader,
                       bool                      decompressFlag,
                       bdlbb::BlobBufferFactory* blobBufferFactory,
                       bslma::Allocator*         allocator);

    /// Copy constructor, from the specified `src`, using the specified
    /// `allocator`.  Needed because BlobObjectProxy doesn't authorize copy
    /// semantic.
    PutMessageIterator(const PutMessageIterator& src,
                       bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.  Needed because
    /// BlobObjectProxy doesn't authorize copy semantic.
    PutMessageIterator& operator=(const PutMessageIterator& rhs);

    /// Advance to the next message.  Return 1 if the new position is valid
    /// and represent a valid message, 0 if iteration has reached the end of
    /// the event, or < 0 if an error was encountered.  Note that if this
    /// method returns 0, this instance goes in an invalid state, and after
    /// that, only valid operations on this instance are assignment, `reset`
    /// and `isValid`.
    int next();

    /// Reset this instance using the specified `blob` and `eventHeader` and
    /// the specified `decompressFlag`. The behaviour is undefined if the
    /// `blob` pointer is null, or the pointed-to blob does not contain
    /// enough bytes to fit at least the `eventHeader`.  Return 0 on
    /// success, and non-zero on error.
    int reset(const bdlbb::Blob* blob,
              const EventHeader& eventHeader,
              bool               decompressFlag);

    /// Point this instance to the specified `blob` using the position and
    /// other meta data from the specified `other` instance.  This method is
    /// useful when it is desired to copy `other` instance into this
    /// instance but the blob being held by `other` instance will not
    /// outlive this instance.  The behavior is undefined unless `blob` is
    /// non-null.  Return 0 on success, and non-zero on error.
    int reset(const bdlbb::Blob* blob, const PutMessageIterator& other);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this PutMessageIterator
    /// to the specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this iterator is initialized and valid, and `next()`
    /// can be called on this instance, or return false in all other cases.
    bool isValid() const;

    /// Return a const reference to the PutHeader currently pointed to by
    /// this iterator.  Behavior is undefined unless `isValid` returns true.
    const PutHeader& header() const;

    /// Return true if the message currently pointed by this iterator has
    /// properties associated with it, false otherwise.  Behavior is
    /// undefined unless `isValid` returns true.
    bool hasMessageProperties() const;

    /// Return `true` if the message currently pointed by this iterator has
    /// a Group Id associated with it, `false` otherwise.  Behavior is
    /// undefined unless `isValid` returns true.
    bool hasMsgGroupId() const;

    /// Return true if the message currently pointed by this iterator has
    /// options associated with it, false otherwise.  Behavior is undefined
    /// unless `isValid` returns true.
    bool hasOptions() const;

    /// Return the size (in bytes) of application data for the message
    /// currently pointed to by this iterator.  Behavior is undefined unless
    /// latest call to `next()` returned 1.  Note that when
    /// `d_decompressFlag` is true, application data includes message
    /// properties and message payload without message padding, but excludes
    /// options. Also, when `d_decompressFlag` is false, this function will
    /// return size of compressed application data without padding.
    int applicationDataSize() const;

    /// Load into the specified `position` the position of the application
    /// data for the message currently pointed to by this iterator.
    /// Behavior is undefined unless latest call to `next()` returned 1.
    /// Note that application data includes message properties and message
    /// payload, but excludes the options.
    int loadApplicationDataPosition(mwcu::BlobPosition* position) const;

    /// Load into the specified `blob` the application data for the message
    /// currently pointed to by this iterator.  Behavior is undefined unless
    /// latest call to `next()` returned 1.  Note that application data
    /// includes message properties and message payload, but excludes
    /// options. When `d_decompressFlag` is true, this returns decompressed
    /// application data and vice-versa.
    int loadApplicationData(bdlbb::Blob* blob) const;

    /// Load into the specified `blob` the options associated with the
    /// message currently under iteration.  Return zero on success, and a
    /// non-zero value otherwise.  Behavior is undefined unless latest call
    /// to `next()` returned 1.  Note that if no options are associated with
    /// the current message, this method will return success.
    int loadOptions(bdlbb::Blob* blob) const;

    /// Return the size (in bytes) of properties for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1.  Note that this length includes padding
    /// and message properties header. Also note that this method returns
    /// zero if no properties are associated with the current message.
    int messagePropertiesSize() const;

    /// Return the size (in bytes) of options for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1.  Note that this length includes
    /// padding.  Also note that this method returns zero if no options are
    /// associated with the current message.
    int optionsSize() const;

    /// Load into the specified `view` a view over the options associated
    /// with the message currently pointed to by this iterator.  Return zero
    /// on success, and a non-zero value otherwise.  Behavior is undefined
    /// unless latest call to `next()` returned 1.  Note that this method
    /// returns success and resets the `iter` if no options are present in
    /// the current message.
    int loadOptionsView(OptionsView* view) const;

    /// Load into the specified `position` the position of properties for
    /// the message currently pointed to by this iterator.  Return zero on
    /// success, non-zero value if no properties are associated with the
    /// current message.  Behavior is undefined unless latest call to
    /// `next()` returned 1.
    int loadMessagePropertiesPosition(mwcu::BlobPosition* position) const;

    /// Load into the specified `blob` the properties for the message
    /// currently pointed to by this iterator.  Behavior is undefined unless
    /// latest call to `next()` returned 1.  Note that padding bytes and
    /// message properties header will also be included.  Also note that
    /// this method returns success if no properties are associated with the
    /// current message, and `blob` will be emptied out in that case. The
    /// blob can be passed to the `streamIn` function of
    /// bmqp::MessageProperties object so as to populate the object.
    int loadMessageProperties(bdlbb::Blob* blob) const;

    /// Load into the specified `properties` the properties associated with
    /// the message currently pointed to by this iterator.  Return zero on
    /// success, and a non-zero value otherwise.  Behavior is undefined
    /// unless latest call to `next()` returned 1.  Note that this method
    /// returns success if no properties are associated with the current
    /// messsage, and `properties` will be cleared out in that case.
    int loadMessageProperties(MessageProperties* properties) const;

    /// Return the size (in bytes) of payload for the message currently
    /// pointed to by this iterator.  Behavior is undefined unless latest
    /// call to `next()` returned 1.
    int messagePayloadSize() const;

    /// Load into the specified `blob` the payload for the message currently
    /// pointed to by this iterator.  Return zero on success, and a non-zero
    /// value otherwise.  Behavior is undefined unless latest call to
    /// `next()` returned 1.
    int loadMessagePayload(bdlbb::Blob* blob) const;

    /// Load into the specified `msgGroupId` the Group Id associated with
    /// the message currently pointed to by this iterator.  Return `true` if
    /// the load was successfully or `false` otherwise.  Behavior is
    /// undefined unless latest call to `next()` returned 1.
    bool extractMsgGroupId(bmqp::Protocol::MsgGroupId* msgGroupId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class PutMessageIterator
// ------------------------

// CREATORS
inline PutMessageIterator::PutMessageIterator(
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator,
    bool                      isDecompressingOldMPs)
: d_blobIter(0, mwcu::BlobPosition(), 0, true)
, d_applicationDataSize(-1)
, d_lazyMessagePayloadSize(-1)
, d_lazyMessagePayloadPosition()
, d_messagePropertiesSize(0)
, d_applicationDataPosition()
, d_optionsSize(0)
, d_optionsPosition()
, d_advanceLength(-1)
, d_optionsView(allocator)
, d_decompressFlag(false)
, d_applicationData(bufferFactory, allocator)
, d_bufferFactory_p(bufferFactory)
, d_isDecompressingOldMPs(isDecompressingOldMPs)
, d_allocator_p(allocator)
{
    // NOTHING
}

inline PutMessageIterator::PutMessageIterator(
    const bdlbb::Blob*        blob,
    const EventHeader&        eventHeader,
    bool                      decompressFlag,
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator)
: d_blobIter(0, mwcu::BlobPosition(), 0, true)  // no def ctor - set in reset
, d_optionsView(allocator)
, d_decompressFlag(decompressFlag)
, d_applicationData(bufferFactory, allocator)
, d_bufferFactory_p(bufferFactory)
, d_isDecompressingOldMPs(false)
, d_allocator_p(allocator)
{
    reset(blob, eventHeader, decompressFlag);
}

inline PutMessageIterator::PutMessageIterator(const PutMessageIterator& src,
                                              bslma::Allocator* allocator)
: d_blobIter(0,
             mwcu::BlobPosition(),
             0,
             true)  // no def ctor - set in copyFrom
, d_applicationData(src.d_bufferFactory_p, allocator)
, d_bufferFactory_p(src.d_bufferFactory_p)
, d_allocator_p(allocator)
{
    copyFrom(src);
}

// MANIPULATORS
inline PutMessageIterator&
PutMessageIterator::operator=(const PutMessageIterator& rhs)
{
    if (this != &rhs) {
        copyFrom(rhs);
    }

    return *this;
}

inline void PutMessageIterator::clear()
{
    d_blobIter.reset(0, mwcu::BlobPosition(), 0, true);
    d_header                     = PutHeader();
    d_applicationDataSize        = -1;
    d_lazyMessagePayloadSize     = -1;
    d_lazyMessagePayloadPosition = mwcu::BlobPosition();
    d_messagePropertiesSize      = 0;
    d_applicationDataPosition    = mwcu::BlobPosition();
    d_optionsSize                = 0;
    d_optionsPosition            = mwcu::BlobPosition();
    d_advanceLength              = -1;
    d_applicationData.removeAll();
    d_optionsView.reset();
}

// ACCESSORS
inline bool PutMessageIterator::isValid() const
{
    return d_advanceLength != -1 && !d_blobIter.atEnd();
}

inline const PutHeader& PutMessageIterator::header() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_header;
}

inline bool PutMessageIterator::hasMessageProperties() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return PutHeaderFlagUtil::isSet(header().flags(),
                                    PutHeaderFlags::e_MESSAGE_PROPERTIES);
}

inline bool PutMessageIterator::hasMsgGroupId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (!hasOptions()) {
        return false;  // RETURN
    }

    // Load options view
    initCachedOptionsView();

    BSLS_ASSERT_SAFE(!d_optionsView.isNull());
    OptionsView& optionsView = d_optionsView.value();
    BSLS_ASSERT_SAFE(optionsView.isValid());

    return optionsView.find(OptionType::e_MSG_GROUP_ID) != optionsView.end();
}

inline bool PutMessageIterator::hasOptions() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(
        (d_optionsSize == 0 && d_optionsPosition == mwcu::BlobPosition()) ||
        (d_optionsSize != 0 && d_optionsPosition != mwcu::BlobPosition()));

    return d_optionsSize > 0;
}

inline int PutMessageIterator::messagePropertiesSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag || d_isDecompressingOldMPs);

    return d_messagePropertiesSize;
}

inline int PutMessageIterator::optionsSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_optionsSize;
}

}  // close package namespace
}  // close enterprise namespace

#endif
