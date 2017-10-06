// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: armycode.proto

#ifndef PROTOBUF_armycode_2eproto__INCLUDED
#define PROTOBUF_armycode_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3002000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3002000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
// @@protoc_insertion_point(includes)
namespace com {
namespace yhsoftlab {
namespace app {
namespace cahelper {
namespace protobuf {
class ArmyCodeAnnounce;
class ArmyCodeAnnounceDefaultTypeInternal;
extern ArmyCodeAnnounceDefaultTypeInternal _ArmyCodeAnnounce_default_instance_;
class ArmyCodeReply;
class ArmyCodeReplyDefaultTypeInternal;
extern ArmyCodeReplyDefaultTypeInternal _ArmyCodeReply_default_instance_;
}  // namespace protobuf
}  // namespace cahelper
}  // namespace app
}  // namespace yhsoftlab
}  // namespace com

namespace com {
namespace yhsoftlab {
namespace app {
namespace cahelper {
namespace protobuf {

namespace protobuf_armycode_2eproto {
// Internal implementation detail -- do not call these.
struct TableStruct {
  static const ::google::protobuf::uint32 offsets[];
  static void InitDefaultsImpl();
  static void Shutdown();
};
void AddDescriptors();
void InitDefaults();
}  // namespace protobuf_armycode_2eproto

// ===================================================================

class ArmyCodeAnnounce : public ::google::protobuf::MessageLite /* @@protoc_insertion_point(class_definition:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce) */ {
 public:
  ArmyCodeAnnounce();
  virtual ~ArmyCodeAnnounce();

  ArmyCodeAnnounce(const ArmyCodeAnnounce& from);

  inline ArmyCodeAnnounce& operator=(const ArmyCodeAnnounce& from) {
    CopyFrom(from);
    return *this;
  }

  static const ArmyCodeAnnounce& default_instance();

  static inline const ArmyCodeAnnounce* internal_default_instance() {
    return reinterpret_cast<const ArmyCodeAnnounce*>(
               &_ArmyCodeAnnounce_default_instance_);
  }

  void Swap(ArmyCodeAnnounce* other);

  // implements Message ----------------------------------------------

  inline ArmyCodeAnnounce* New() const PROTOBUF_FINAL { return New(NULL); }

  ArmyCodeAnnounce* New(::google::protobuf::Arena* arena) const PROTOBUF_FINAL;
  void CheckTypeAndMergeFrom(const ::google::protobuf::MessageLite& from)
    PROTOBUF_FINAL;
  void CopyFrom(const ArmyCodeAnnounce& from);
  void MergeFrom(const ArmyCodeAnnounce& from);
  void Clear() PROTOBUF_FINAL;
  bool IsInitialized() const PROTOBUF_FINAL;

  size_t ByteSizeLong() const PROTOBUF_FINAL;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) PROTOBUF_FINAL;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const PROTOBUF_FINAL;
  void DiscardUnknownFields();
  int GetCachedSize() const PROTOBUF_FINAL { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ArmyCodeAnnounce* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::std::string GetTypeName() const PROTOBUF_FINAL;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string code = 1;
  void clear_code();
  static const int kCodeFieldNumber = 1;
  const ::std::string& code() const;
  void set_code(const ::std::string& value);
  #if LANG_CXX11
  void set_code(::std::string&& value);
  #endif
  void set_code(const char* value);
  void set_code(const char* value, size_t size);
  ::std::string* mutable_code();
  ::std::string* release_code();
  void set_allocated_code(::std::string* code);

  // string fbid = 2;
  void clear_fbid();
  static const int kFbidFieldNumber = 2;
  const ::std::string& fbid() const;
  void set_fbid(const ::std::string& value);
  #if LANG_CXX11
  void set_fbid(::std::string&& value);
  #endif
  void set_fbid(const char* value);
  void set_fbid(const char* value, size_t size);
  ::std::string* mutable_fbid();
  ::std::string* release_fbid();
  void set_allocated_fbid(::std::string* fbid);

  // @@protoc_insertion_point(class_scope:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce)
 private:

  ::google::protobuf::internal::InternalMetadataWithArenaLite _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr code_;
  ::google::protobuf::internal::ArenaStringPtr fbid_;
  mutable int _cached_size_;
  friend struct  protobuf_armycode_2eproto::TableStruct;
};
// -------------------------------------------------------------------

class ArmyCodeReply : public ::google::protobuf::MessageLite /* @@protoc_insertion_point(class_definition:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply) */ {
 public:
  ArmyCodeReply();
  virtual ~ArmyCodeReply();

  ArmyCodeReply(const ArmyCodeReply& from);

  inline ArmyCodeReply& operator=(const ArmyCodeReply& from) {
    CopyFrom(from);
    return *this;
  }

  static const ArmyCodeReply& default_instance();

  static inline const ArmyCodeReply* internal_default_instance() {
    return reinterpret_cast<const ArmyCodeReply*>(
               &_ArmyCodeReply_default_instance_);
  }

  void Swap(ArmyCodeReply* other);

  // implements Message ----------------------------------------------

  inline ArmyCodeReply* New() const PROTOBUF_FINAL { return New(NULL); }

  ArmyCodeReply* New(::google::protobuf::Arena* arena) const PROTOBUF_FINAL;
  void CheckTypeAndMergeFrom(const ::google::protobuf::MessageLite& from)
    PROTOBUF_FINAL;
  void CopyFrom(const ArmyCodeReply& from);
  void MergeFrom(const ArmyCodeReply& from);
  void Clear() PROTOBUF_FINAL;
  bool IsInitialized() const PROTOBUF_FINAL;

  size_t ByteSizeLong() const PROTOBUF_FINAL;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) PROTOBUF_FINAL;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const PROTOBUF_FINAL;
  void DiscardUnknownFields();
  int GetCachedSize() const PROTOBUF_FINAL { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ArmyCodeReply* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::std::string GetTypeName() const PROTOBUF_FINAL;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // repeated .com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce announces = 1;
  int announces_size() const;
  void clear_announces();
  static const int kAnnouncesFieldNumber = 1;
  const ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce& announces(int index) const;
  ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce* mutable_announces(int index);
  ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce* add_announces();
  ::google::protobuf::RepeatedPtrField< ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce >*
      mutable_announces();
  const ::google::protobuf::RepeatedPtrField< ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce >&
      announces() const;

  // @@protoc_insertion_point(class_scope:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply)
 private:

  ::google::protobuf::internal::InternalMetadataWithArenaLite _internal_metadata_;
  ::google::protobuf::RepeatedPtrField< ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce > announces_;
  mutable int _cached_size_;
  friend struct  protobuf_armycode_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// ArmyCodeAnnounce

// string code = 1;
inline void ArmyCodeAnnounce::clear_code() {
  code_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ArmyCodeAnnounce::code() const {
  // @@protoc_insertion_point(field_get:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
  return code_.GetNoArena();
}
inline void ArmyCodeAnnounce::set_code(const ::std::string& value) {
  
  code_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
}
#if LANG_CXX11
inline void ArmyCodeAnnounce::set_code(::std::string&& value) {
  
  code_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
}
#endif
inline void ArmyCodeAnnounce::set_code(const char* value) {
  
  code_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
}
inline void ArmyCodeAnnounce::set_code(const char* value, size_t size) {
  
  code_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
}
inline ::std::string* ArmyCodeAnnounce::mutable_code() {
  
  // @@protoc_insertion_point(field_mutable:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
  return code_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ArmyCodeAnnounce::release_code() {
  // @@protoc_insertion_point(field_release:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
  
  return code_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ArmyCodeAnnounce::set_allocated_code(::std::string* code) {
  if (code != NULL) {
    
  } else {
    
  }
  code_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), code);
  // @@protoc_insertion_point(field_set_allocated:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.code)
}

// string fbid = 2;
inline void ArmyCodeAnnounce::clear_fbid() {
  fbid_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& ArmyCodeAnnounce::fbid() const {
  // @@protoc_insertion_point(field_get:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
  return fbid_.GetNoArena();
}
inline void ArmyCodeAnnounce::set_fbid(const ::std::string& value) {
  
  fbid_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
}
#if LANG_CXX11
inline void ArmyCodeAnnounce::set_fbid(::std::string&& value) {
  
  fbid_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
}
#endif
inline void ArmyCodeAnnounce::set_fbid(const char* value) {
  
  fbid_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
}
inline void ArmyCodeAnnounce::set_fbid(const char* value, size_t size) {
  
  fbid_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
}
inline ::std::string* ArmyCodeAnnounce::mutable_fbid() {
  
  // @@protoc_insertion_point(field_mutable:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
  return fbid_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ArmyCodeAnnounce::release_fbid() {
  // @@protoc_insertion_point(field_release:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
  
  return fbid_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ArmyCodeAnnounce::set_allocated_fbid(::std::string* fbid) {
  if (fbid != NULL) {
    
  } else {
    
  }
  fbid_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), fbid);
  // @@protoc_insertion_point(field_set_allocated:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce.fbid)
}

// -------------------------------------------------------------------

// ArmyCodeReply

// repeated .com.yhsoftlab.app.cahelper.protobuf.ArmyCodeAnnounce announces = 1;
inline int ArmyCodeReply::announces_size() const {
  return announces_.size();
}
inline void ArmyCodeReply::clear_announces() {
  announces_.Clear();
}
inline const ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce& ArmyCodeReply::announces(int index) const {
  // @@protoc_insertion_point(field_get:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply.announces)
  return announces_.Get(index);
}
inline ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce* ArmyCodeReply::mutable_announces(int index) {
  // @@protoc_insertion_point(field_mutable:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply.announces)
  return announces_.Mutable(index);
}
inline ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce* ArmyCodeReply::add_announces() {
  // @@protoc_insertion_point(field_add:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply.announces)
  return announces_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce >*
ArmyCodeReply::mutable_announces() {
  // @@protoc_insertion_point(field_mutable_list:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply.announces)
  return &announces_;
}
inline const ::google::protobuf::RepeatedPtrField< ::com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce >&
ArmyCodeReply::announces() const {
  // @@protoc_insertion_point(field_list:com.yhsoftlab.app.cahelper.protobuf.ArmyCodeReply.announces)
  return announces_;
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)


}  // namespace protobuf
}  // namespace cahelper
}  // namespace app
}  // namespace yhsoftlab
}  // namespace com

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_armycode_2eproto__INCLUDED
