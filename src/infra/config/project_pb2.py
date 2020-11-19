# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: project.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='project.proto',
  package='milo',
  syntax='proto3',
  serialized_options=_b('Z\006config'),
  serialized_pb=_b('\n\rproject.proto\x12\x04milo\"a\n\x07Project\x12\x1f\n\x08\x63onsoles\x18\x02 \x03(\x0b\x32\r.milo.Console\x12\x1d\n\x07headers\x18\x03 \x03(\x0b\x32\x0c.milo.Header\x12\x10\n\x08logo_url\x18\x04 \x01(\tJ\x04\x08\x01\x10\x02\".\n\x04Link\x12\x0c\n\x04text\x18\x01 \x01(\t\x12\x0b\n\x03url\x18\x02 \x01(\t\x12\x0b\n\x03\x61lt\x18\x03 \x01(\t\"#\n\x06Oncall\x12\x0c\n\x04name\x18\x01 \x01(\t\x12\x0b\n\x03url\x18\x02 \x01(\t\"4\n\tLinkGroup\x12\x0c\n\x04name\x18\x01 \x01(\t\x12\x19\n\x05links\x18\x02 \x03(\x0b\x32\n.milo.Link\"E\n\x13\x43onsoleSummaryGroup\x12\x19\n\x05title\x18\x01 \x01(\x0b\x32\n.milo.Link\x12\x13\n\x0b\x63onsole_ids\x18\x02 \x03(\t\"\xa0\x01\n\x06Header\x12\x1d\n\x07oncalls\x18\x01 \x03(\x0b\x32\x0c.milo.Oncall\x12\x1e\n\x05links\x18\x02 \x03(\x0b\x32\x0f.milo.LinkGroup\x12\x31\n\x0e\x63onsole_groups\x18\x03 \x03(\x0b\x32\x19.milo.ConsoleSummaryGroup\x12\x18\n\x10tree_status_host\x18\x04 \x01(\t\x12\n\n\x02id\x18\x05 \x01(\t\"\xa7\x02\n\x07\x43onsole\x12\n\n\x02id\x18\x01 \x01(\t\x12\x0c\n\x04name\x18\x02 \x01(\t\x12\x10\n\x08repo_url\x18\x03 \x01(\t\x12\x0c\n\x04refs\x18\x0e \x03(\t\x12\x13\n\x0b\x65xclude_ref\x18\r \x01(\t\x12\x15\n\rmanifest_name\x18\x05 \x01(\t\x12\x1f\n\x08\x62uilders\x18\x06 \x03(\x0b\x32\r.milo.Builder\x12\x13\n\x0b\x66\x61vicon_url\x18\x07 \x01(\t\x12\x1c\n\x06header\x18\t \x01(\x0b\x32\x0c.milo.Header\x12\x11\n\theader_id\x18\n \x01(\t\x12#\n\x1binclude_experimental_builds\x18\x0b \x01(\x08\x12\x19\n\x11\x62uilder_view_only\x18\x0c \x01(\x08J\x04\x08\x08\x10\tJ\x04\x08\x04\x10\x05R\x03ref\"=\n\x07\x42uilder\x12\x0c\n\x04name\x18\x01 \x03(\t\x12\x10\n\x08\x63\x61tegory\x18\x02 \x01(\t\x12\x12\n\nshort_name\x18\x03 \x01(\tB\x08Z\x06\x63onfigb\x06proto3')
)




_PROJECT = _descriptor.Descriptor(
  name='Project',
  full_name='milo.Project',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='consoles', full_name='milo.Project.consoles', index=0,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='headers', full_name='milo.Project.headers', index=1,
      number=3, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='logo_url', full_name='milo.Project.logo_url', index=2,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=23,
  serialized_end=120,
)


_LINK = _descriptor.Descriptor(
  name='Link',
  full_name='milo.Link',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='text', full_name='milo.Link.text', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='url', full_name='milo.Link.url', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='alt', full_name='milo.Link.alt', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=122,
  serialized_end=168,
)


_ONCALL = _descriptor.Descriptor(
  name='Oncall',
  full_name='milo.Oncall',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='name', full_name='milo.Oncall.name', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='url', full_name='milo.Oncall.url', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=170,
  serialized_end=205,
)


_LINKGROUP = _descriptor.Descriptor(
  name='LinkGroup',
  full_name='milo.LinkGroup',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='name', full_name='milo.LinkGroup.name', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='links', full_name='milo.LinkGroup.links', index=1,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=207,
  serialized_end=259,
)


_CONSOLESUMMARYGROUP = _descriptor.Descriptor(
  name='ConsoleSummaryGroup',
  full_name='milo.ConsoleSummaryGroup',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='title', full_name='milo.ConsoleSummaryGroup.title', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='console_ids', full_name='milo.ConsoleSummaryGroup.console_ids', index=1,
      number=2, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=261,
  serialized_end=330,
)


_HEADER = _descriptor.Descriptor(
  name='Header',
  full_name='milo.Header',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='oncalls', full_name='milo.Header.oncalls', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='links', full_name='milo.Header.links', index=1,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='console_groups', full_name='milo.Header.console_groups', index=2,
      number=3, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='tree_status_host', full_name='milo.Header.tree_status_host', index=3,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='id', full_name='milo.Header.id', index=4,
      number=5, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=333,
  serialized_end=493,
)


_CONSOLE = _descriptor.Descriptor(
  name='Console',
  full_name='milo.Console',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='milo.Console.id', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='name', full_name='milo.Console.name', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='repo_url', full_name='milo.Console.repo_url', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='refs', full_name='milo.Console.refs', index=3,
      number=14, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='exclude_ref', full_name='milo.Console.exclude_ref', index=4,
      number=13, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='manifest_name', full_name='milo.Console.manifest_name', index=5,
      number=5, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='builders', full_name='milo.Console.builders', index=6,
      number=6, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='favicon_url', full_name='milo.Console.favicon_url', index=7,
      number=7, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='header', full_name='milo.Console.header', index=8,
      number=9, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='header_id', full_name='milo.Console.header_id', index=9,
      number=10, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='include_experimental_builds', full_name='milo.Console.include_experimental_builds', index=10,
      number=11, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='builder_view_only', full_name='milo.Console.builder_view_only', index=11,
      number=12, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=496,
  serialized_end=791,
)


_BUILDER = _descriptor.Descriptor(
  name='Builder',
  full_name='milo.Builder',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='name', full_name='milo.Builder.name', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='category', full_name='milo.Builder.category', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='short_name', full_name='milo.Builder.short_name', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=793,
  serialized_end=854,
)

_PROJECT.fields_by_name['consoles'].message_type = _CONSOLE
_PROJECT.fields_by_name['headers'].message_type = _HEADER
_LINKGROUP.fields_by_name['links'].message_type = _LINK
_CONSOLESUMMARYGROUP.fields_by_name['title'].message_type = _LINK
_HEADER.fields_by_name['oncalls'].message_type = _ONCALL
_HEADER.fields_by_name['links'].message_type = _LINKGROUP
_HEADER.fields_by_name['console_groups'].message_type = _CONSOLESUMMARYGROUP
_CONSOLE.fields_by_name['builders'].message_type = _BUILDER
_CONSOLE.fields_by_name['header'].message_type = _HEADER
DESCRIPTOR.message_types_by_name['Project'] = _PROJECT
DESCRIPTOR.message_types_by_name['Link'] = _LINK
DESCRIPTOR.message_types_by_name['Oncall'] = _ONCALL
DESCRIPTOR.message_types_by_name['LinkGroup'] = _LINKGROUP
DESCRIPTOR.message_types_by_name['ConsoleSummaryGroup'] = _CONSOLESUMMARYGROUP
DESCRIPTOR.message_types_by_name['Header'] = _HEADER
DESCRIPTOR.message_types_by_name['Console'] = _CONSOLE
DESCRIPTOR.message_types_by_name['Builder'] = _BUILDER
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Project = _reflection.GeneratedProtocolMessageType('Project', (_message.Message,), dict(
  DESCRIPTOR = _PROJECT,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Project)
  ))
_sym_db.RegisterMessage(Project)

Link = _reflection.GeneratedProtocolMessageType('Link', (_message.Message,), dict(
  DESCRIPTOR = _LINK,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Link)
  ))
_sym_db.RegisterMessage(Link)

Oncall = _reflection.GeneratedProtocolMessageType('Oncall', (_message.Message,), dict(
  DESCRIPTOR = _ONCALL,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Oncall)
  ))
_sym_db.RegisterMessage(Oncall)

LinkGroup = _reflection.GeneratedProtocolMessageType('LinkGroup', (_message.Message,), dict(
  DESCRIPTOR = _LINKGROUP,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.LinkGroup)
  ))
_sym_db.RegisterMessage(LinkGroup)

ConsoleSummaryGroup = _reflection.GeneratedProtocolMessageType('ConsoleSummaryGroup', (_message.Message,), dict(
  DESCRIPTOR = _CONSOLESUMMARYGROUP,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.ConsoleSummaryGroup)
  ))
_sym_db.RegisterMessage(ConsoleSummaryGroup)

Header = _reflection.GeneratedProtocolMessageType('Header', (_message.Message,), dict(
  DESCRIPTOR = _HEADER,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Header)
  ))
_sym_db.RegisterMessage(Header)

Console = _reflection.GeneratedProtocolMessageType('Console', (_message.Message,), dict(
  DESCRIPTOR = _CONSOLE,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Console)
  ))
_sym_db.RegisterMessage(Console)

Builder = _reflection.GeneratedProtocolMessageType('Builder', (_message.Message,), dict(
  DESCRIPTOR = _BUILDER,
  __module__ = 'project_pb2'
  # @@protoc_insertion_point(class_scope:milo.Builder)
  ))
_sym_db.RegisterMessage(Builder)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)