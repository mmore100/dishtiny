#pragma once

#include "base/Ptr.h"

#include "Config.h"
#include "FrameHardware.h"
#include "Manager.h"

class LibraryEvent {

public:

  using hardware_t = Config::hardware_t;
  using event_lib_t = Config::event_lib_t;
  using event_t = event_lib_t::event_t;

  static const event_lib_t& Make(const Config &cfg) {

    static event_lib_t el;

    if (el.GetSize() == 0) {

      el.AddEvent(
        "EnvTrigger",
        [](hardware_t &hw, const event_t & event){
          hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
        }
      );

      el.RegisterDispatchFun(
        "EnvTrigger",
        [](hardware_t &hw, const event_t & event){
          hw.QueueEvent(event);
        }
      );

      el.AddEvent(
        "SendMsgExternal",
        [](hardware_t & hw, const event_t & event){
          hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
        },
        "Send message event."
      );

      el.AddEvent(
        "SendMsgInternal",
        [](hardware_t & hw, const event_t & event){
          hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
        },
        "Send message event."
      );

      el.AddEvent(
        "SendMsgSpiker",
        [](hardware_t & hw, const event_t & event){
          hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
        },
        "Send message event."
      );

      el.AddEvent(
        "SendSpikeMsg",
        [](hardware_t & hw, const event_t & event){
          hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
        },
        "Send message event."
      );

      el.RegisterDispatchFun(
        "SendMsgExternal",
        [](hardware_t & hw, const event_t & event) {

          FrameHardware &fh = *hw.GetTrait();
          Manager &man = fh.Cell().Man();
          const size_t outgoing_dir = fh.GetMsgDir();
          const size_t dest = fh.Cell().GetNeigh(outgoing_dir);

          man.Inbox(dest).TakeMessage(event, Cardi::Opp[outgoing_dir]);
        }
      );

      el.RegisterDispatchFun(
        "SendMsgInternal",
        [](hardware_t & hw, const event_t & event) {

          FrameHardware &fh = *hw.GetTrait();
          const size_t dir = fh.GetMsgDir();

          fh.Cell().GetFrameHardware(dir).QueueInternalMessage(event);
        }
      );

      el.RegisterDispatchFun(
        "SendMsgSpiker",
        [](hardware_t & hw, const event_t & event) {

          FrameHardware &fh = *hw.GetTrait();

          fh.Cell().GetSpiker().QueueInternalMessage(event);
        }
      );

      el.RegisterDispatchFun(
        "SendSpikeMsg",
        [](hardware_t & hw, const event_t & event) {

          FrameHardware &fh = *hw.GetTrait();
          Manager &man = fh.Cell().Man();
          const size_t pos = fh.Cell().GetPos();

          for (auto & [dest, target] : man.Connection(pos).ViewDeveloped()) {
            man.Inbox(dest).TakeMessage(event, Cardi::Dir::NumDirs);
          }

        }
      );

      std::cout << "Event Library Size: " << el.GetSize() << std::endl;

    }

    return el;

  }

};
