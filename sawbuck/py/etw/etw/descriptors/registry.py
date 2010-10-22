#!python
# Copyright 2010 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# DO NOT EDIT. This is an ETW event descriptor file generated by
# sawbuck/py/generate_descriptor.py. It contains event descriptions for
# MOF GUID {ae53722e-c863-11d2-8659-00c04fa321a1}.

from etw.descriptors import event, field


class Event(object):
  GUID = '{ae53722e-c863-11d2-8659-00c04fa321a1}'
  Create = (GUID, 10)
  Open = (GUID, 11)
  Delete = (GUID, 12)
  Query = (GUID, 13)
  SetValue = (GUID, 14)
  DeleteValue = (GUID, 15)
  QueryValue = (GUID, 16)
  EnumerateKey = (GUID, 17)
  EnumerateValueKey = (GUID, 18)
  QueryMultipleValue = (GUID, 19)
  SetInformation = (GUID, 20)
  Flush = (GUID, 21)
  RunDown = (GUID, 22)
  KCBDelete = (GUID, 23)
  KCBRundownBegin = (GUID, 24)
  KCBRundownEnd = (GUID, 25)
  Virtualize = (GUID, 26)
  Close = (GUID, 27)
  SetSecurity = (GUID, 28)
  QuerySecurity = (GUID, 29)
  TxRCommit = (GUID, 30)
  TxRPrepare = (GUID, 31)
  TxRRollback = (GUID, 32)
  Counters = (GUID, 34)
  Config = (GUID, 35)


class Registry(event.EventCategory):
  GUID = Event.GUID
  VERSION = 2

  class Registry_Config(event.EventClass):
    _event_types_ = [Event.Config]
    _fields_ = [('CurrentControlSet', field.UInt32)]

  class Registry_Counters(event.EventClass):
    _event_types_ = [Event.Counters]
    _fields_ = [('Counter1', field.UInt64),
                ('Counter2', field.UInt64),
                ('Counter3', field.UInt64),
                ('Counter4', field.UInt64),
                ('Counter5', field.UInt64),
                ('Counter6', field.UInt64),
                ('Counter7', field.UInt64),
                ('Counter8', field.UInt64),
                ('Counter9', field.UInt64),
                ('Counter10', field.UInt64),
                ('Counter11', field.UInt64)]

  class Registry_TypeGroup1(event.EventClass):
    _event_types_ = [Event.Close,
                     Event.Create,
                     Event.Delete,
                     Event.DeleteValue,
                     Event.EnumerateKey,
                     Event.EnumerateValueKey,
                     Event.Flush,
                     Event.KCBCreate,
                     Event.KCBDelete,
                     Event.KCBRundownBegin,
                     Event.KCBRundownEnd,
                     Event.Open,
                     Event.Query,
                     Event.QueryMultipleValue,
                     Event.QuerySecurity,
                     Event.QueryValue,
                     Event.SetInformation,
                     Event.SetSecurity,
                     Event.SetValue,
                     Event.Virtualize]
    _fields_ = [('InitialTime', field.Int64),
                ('Status', field.UInt32),
                ('Index', field.UInt32),
                ('KeyHandle', field.Pointer),
                ('KeyName', field.WString)]

  class Registry_TxR(event.EventClass):
    _event_types_ = [Event.TxRCommit,
                     Event.TxRPrepare,
                     Event.TxRRollback]
    _fields_ = [('TxrGUID', field.UInt8),
                ('Status', field.UInt32),
                ('UowCount', field.UInt32),
                ('OperationTime', field.UInt64),
                ('Hive', field.WString)]


class Registry_V0(event.EventCategory):
  GUID = Event.GUID
  VERSION = 0

  class Registry_V0_TypeGroup1(event.EventClass):
    _event_types_ = [Event.Create,
                     Event.Delete,
                     Event.DeleteValue,
                     Event.EnumerateKey,
                     Event.EnumerateValueKey,
                     Event.Flush,
                     Event.Open,
                     Event.Query,
                     Event.QueryMultipleValue,
                     Event.QueryValue,
                     Event.SetInformation,
                     Event.SetValue]
    _fields_ = [('Status', field.Pointer),
                ('KeyHandle', field.Pointer),
                ('ElapsedTime', field.Int64),
                ('KeyName', field.WString)]


class Registry_V1(event.EventCategory):
  GUID = Event.GUID
  VERSION = 1

  class Registry_V1_TypeGroup1(event.EventClass):
    _event_types_ = [Event.Create,
                     Event.Delete,
                     Event.DeleteValue,
                     Event.EnumerateKey,
                     Event.EnumerateValueKey,
                     Event.Flush,
                     Event.Open,
                     Event.Query,
                     Event.QueryMultipleValue,
                     Event.QueryValue,
                     Event.RunDown,
                     Event.SetInformation,
                     Event.SetValue]
    _fields_ = [('Status', field.Pointer),
                ('KeyHandle', field.Pointer),
                ('ElapsedTime', field.Int64),
                ('Index', field.UInt32),
                ('KeyName', field.WString)]
