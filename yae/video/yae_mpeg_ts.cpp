// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Nov  2 12:04:37 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <time.h>

// yae includes:
#include "yae/api/yae_log.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_mpeg_ts.h"


namespace yae
{

  namespace mpeg_ts
  {

    //----------------------------------------------------------------
    // AdaptationField::Extension::Extension
    //
    AdaptationField::Extension::Extension():
      adaptation_field_extension_length_(0),
      ltw_flag_(0),
      piecewise_rate_flag_(0),
      seamless_splice_flag_(0),
      reserved1_(0),
      ltw_valid_flag_(0),
      ltw_offset_(0),
      reserved2_(0),
      piecewise_rate_(0),
      splice_type_(0),
      dts_next_au_32_30_(0),
      marker1_(0),
      dts_next_au_29_15_(0),
      marker2_(0),
      dts_next_au_14_00_(0),
      marker3_(0)
    {}

    //----------------------------------------------------------------
    // AdaptationField::Extension::load
    //
    void
    AdaptationField::Extension::load(IBitstream & bin)
    {
      adaptation_field_extension_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      ltw_flag_ = bin.read(1);
      piecewise_rate_flag_ = bin.read(1);
      seamless_splice_flag_ = bin.read(1);
      reserved1_ = bin.read(5);
      YAE_THROW_IF(reserved1_ != 0x1F);

      if (ltw_flag_)
      {
        ltw_valid_flag_ = bin.read(1);
        ltw_offset_ = bin.read(15);
      }

      if (piecewise_rate_flag_)
      {
        reserved2_ = bin.read(2);
        YAE_THROW_IF(reserved2_ != 0x3);

        piecewise_rate_ = bin.read(22);
      }

      if (seamless_splice_flag_)
      {
        splice_type_ = bin.read(4);
        dts_next_au_32_30_ = bin.read(3);
        marker1_ = bin.read(1);
        YAE_THROW_IF(marker1_ != 1);

        dts_next_au_29_15_ = bin.read(15);
        marker2_ = bin.read(1);
        YAE_THROW_IF(marker2_ != 1);

        dts_next_au_14_00_ = bin.read(15);
        marker3_ = bin.read(1);
        YAE_THROW_IF(marker3_ != 1);
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      reserved_ = bin.read_bytes(adaptation_field_extension_length_ -
                                 consumed_bytes);
    }


    //----------------------------------------------------------------
    // AdaptationField::AdaptationField
    //
    AdaptationField::AdaptationField():
      adaptation_field_length_(0),
      discontinuity_indicator_(0),
      random_access_indicator_(0),
      elementary_stream_priority_indicator_(0),
      pcr_flag_(0),
      opcr_flag_(0),
      splicing_point_flag_(0),
      transport_private_data_flag_(0),
      adaptation_field_extension_flag_(0),
      program_clock_reference_base_(0),
      program_clock_reference_reserved_(0),
      program_clock_reference_extension_(0),
      original_program_clock_reference_base_(0),
      original_program_clock_reference_reserved_(0),
      original_program_clock_reference_extension_(0),
      splice_countdown_(0),
      transport_private_data_length_(0)
    {}

    //----------------------------------------------------------------
    // AdaptationField::load
    //
    void
    AdaptationField::load(IBitstream & bin)
    {
      adaptation_field_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      if (adaptation_field_length_)
      {
        discontinuity_indicator_ = bin.read(1);
        random_access_indicator_ = bin.read(1);
        elementary_stream_priority_indicator_ = bin.read(1);
        pcr_flag_ = bin.read(1);
        opcr_flag_ = bin.read(1);
        splicing_point_flag_ = bin.read(1);
        transport_private_data_flag_ = bin.read(1);
        adaptation_field_extension_flag_ = bin.read(1);

        if (pcr_flag_)
        {
          program_clock_reference_base_ = bin.read(33);
          program_clock_reference_reserved_ = bin.read(6);
          // YAE_THROW_IF(program_clock_reference_reserved_ != 0x3F);
          program_clock_reference_extension_ = bin.read(9);
        }

        if (opcr_flag_)
        {
          original_program_clock_reference_base_ = bin.read(33);
          original_program_clock_reference_reserved_ = bin.read(6);
          YAE_THROW_IF(original_program_clock_reference_reserved_ != 0x3F);
          original_program_clock_reference_extension_ = bin.read(9);
        }

        if (splicing_point_flag_)
        {
          splice_countdown_ = bin.read(8);
        }

        if (transport_private_data_flag_)
        {
          transport_private_data_length_ = bin.read(8);
          transport_private_data_ =
            bin.read_bytes(transport_private_data_length_);
        }

        if (adaptation_field_extension_flag_)
        {
          extension_ = Extension();
          extension_->load(bin);
        }

        std::size_t end_pos = bin.position();
        std::size_t consumed = end_pos - start_pos;
        YAE_THROW_IF((consumed & 0x7) != 0);

        std::size_t consumed_bytes = consumed >> 3;
        stuffing_ = bin.read_bytes(adaptation_field_length_ -
                                   consumed_bytes);
      }
    }


    //----------------------------------------------------------------
    // TSPacket::TSPacket
    //
    TSPacket::TSPacket():
      sync_byte_(0x47),
      transport_error_indicator_(0),
      payload_unit_start_indicator_(0),
      transport_priority_(0),
      pid_(0x1FFF),
      transport_scrambling_control_(0),
      adaptation_field_control_(0),
      continuity_counter_(0)
    {}

    //----------------------------------------------------------------
    // TSPacket::load
    //
    void
    TSPacket::load(IBitstream & bin, Context & ctx)
    {
      sync_byte_ = bin.read(8);
      YAE_THROW_IF(sync_byte_ != 0x47);

      transport_error_indicator_ = bin.read(1);
      payload_unit_start_indicator_ = bin.read(1);
      transport_priority_ = bin.read(1);

      pid_ = bin.read(13);

      transport_scrambling_control_ = bin.read(2);
      adaptation_field_control_ = bin.read(2);
      continuity_counter_ = bin.read(4);

      if ((adaptation_field_control_ & 2) == 2)
      {
        adaptation_field_ = AdaptationField();
        adaptation_field_->load(bin);
      }

      if ((adaptation_field_control_ & 1) == 1)
      {
        payload_ = bin.read_remaining_bytes();
      }
    }


    //----------------------------------------------------------------
    // SystemHeader::SystemHeader
    //
    SystemHeader::SystemHeader():
      system_header_start_code_(0),
      header_length_(0),
      marker1_(0),
      rate_bound_(0),
      marker2_(0),
      audio_bound_(0),
      fixed_flag_(0),
      csps_flag_(0),
      system_audio_lock_flag_(0),
      system_video_lock_flag_(0),
      marker3_(0),
      video_bound_(0),
      packet_rate_restriction_flag_(0),
      reserved_(0)
    {}

    //----------------------------------------------------------------
    // SystemHeader::load
    //
    void
    SystemHeader::load(IBitstream & bin)
    {
      system_header_start_code_ = bin.read(32);
      YAE_THROW_IF(system_header_start_code_ != 0x000001BB);

      header_length_ = bin.read(16);
      marker1_ = bin.read(1);
      YAE_THROW_IF(marker1_ != 1);

      rate_bound_ = bin.read(22);
      marker2_ = bin.read(1);
      YAE_THROW_IF(marker2_ != 1);

      audio_bound_ = bin.read(6);
      fixed_flag_ = bin.read(1);
      csps_flag_ = bin.read(1);

      system_audio_lock_flag_ = bin.read(1);
      system_video_lock_flag_ = bin.read(1);
      marker3_ = bin.read(1);
      YAE_THROW_IF(marker3_ != 1);

      video_bound_ = bin.read(5);

      packet_rate_restriction_flag_ = bin.read(1);
      reserved_ = bin.read(7);
      YAE_THROW_IF(reserved_ != 0x7F);

      while (bin.next_bits(1, 1))
      {
        ext_.push_back(Ext());
        Ext & ext = ext_.back();
        ext.load(bin);
      }
    }

    //----------------------------------------------------------------
    // SystemHeader::Ext::Ext
    //
    SystemHeader::Ext::Ext():
      stream_id_(0),
      const1_11_(0),
      const_0000000_(0),
      stream_id_extension_(0),
      const_10110110_(0),
      const_11_(0),
      pstd_buffer_bound_scale_(0),
      pstd_buffer_size_bound_(0)
    {}

    //----------------------------------------------------------------
    // SystemHeader::Ext::load
    //
    void
    SystemHeader::Ext::load(IBitstream & bin)
    {
      stream_id_ = bin.read(8);
      if (stream_id_ == 0xB7)
      {
        const1_11_ = bin.read(2);
        YAE_THROW_IF(const1_11_ != 3);

        const_0000000_  = bin.read(7);
        YAE_THROW_IF(const_0000000_ != 0);

        stream_id_extension_ = bin.read(7);

        const_10110110_ = bin.read(8);
        YAE_THROW_IF(const_10110110_ != 0xB6);
      }

      const_11_ = bin.read(2);
      YAE_THROW_IF(const_11_ != 3);

      pstd_buffer_bound_scale_ = bin.read(1);
      pstd_buffer_size_bound_ = bin.read(13);
    }


    //----------------------------------------------------------------
    // PackHeader::PackHeader
    //
    PackHeader::PackHeader():
      pack_start_code_(0),
      pack_const_01_(0),
      system_clock_reference_base_32_30_(0),
      system_clock_reference_marker1_(0),
      system_clock_reference_base_29_15_(0),
      system_clock_reference_marker2_(0),
      system_clock_reference_base_14_00_(0),
      system_clock_reference_marker3_(0),
      system_clock_reference_extension_(0),
      system_clock_reference_marker4_(0),
      program_mux_rate_(0),
      marker1_(0),
      marker2_(0),
      reserved_(0),
      pack_stuffing_length_(0)
    {}

    //----------------------------------------------------------------
    // PackHeader::load
    //
    void PackHeader::load(IBitstream & bin)
    {
      pack_start_code_ = bin.read(32);
      YAE_THROW_IF(pack_start_code_ != 0x000001BB);

      pack_const_01_ = bin.read(2);
      YAE_THROW_IF(pack_const_01_ != 1);

      system_clock_reference_base_32_30_ = bin.read(3);
      system_clock_reference_marker1_ = bin.read(1);
      YAE_THROW_IF(system_clock_reference_marker1_ != 1);

      system_clock_reference_base_29_15_ = bin.read(15);
      system_clock_reference_marker2_ = bin.read(1);
      YAE_THROW_IF(system_clock_reference_marker2_ != 1);

      system_clock_reference_base_14_00_ = bin.read(15);
      system_clock_reference_marker3_ = bin.read(1);
      YAE_THROW_IF(system_clock_reference_marker3_ != 1);

      system_clock_reference_extension_ = bin.read(9);
      system_clock_reference_marker4_ = bin.read(1);
      YAE_THROW_IF(system_clock_reference_marker4_ != 1);

      program_mux_rate_ = bin.read(22);
      marker1_ = bin.read(1);
      YAE_THROW_IF(marker1_ != 1);
      marker2_ = bin.read(1);
      YAE_THROW_IF(marker2_ != 1);

      reserved_ = bin.read(5);
      YAE_THROW_IF(reserved_ != 0x1F);

      pack_stuffing_length_ = bin.read(3);
      stuffing_ = bin.read_bytes(pack_stuffing_length_);

      if (bin.next_bits(32, 0x000001BB))
      {
        system_header_ = SystemHeader();
        SystemHeader & system_header = *system_header_;
        system_header.load(bin);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PESPacket
    //
    PESPacket::PESPacket():
      packet_start_code_prefix_(0),
      stream_id_(0),
      pes_packet_length_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::load
    //
    void
    PESPacket::load(IBitstream & bin)
    {
      packet_start_code_prefix_ = bin.read(24);
      YAE_THROW_IF(packet_start_code_prefix_ != 0x000001);

      stream_id_ = bin.read(8);
      pes_packet_length_ = bin.read(16);
      YAE_THROW_IF(!bin.has_enough_bytes(pes_packet_length_));

      std::size_t start_pos = bin.position();

      if (stream_id_ != STREAM_ID_PROGRAM_STREAM_MAP &&
          stream_id_ != STREAM_ID_PADDING_STREAM  &&
          stream_id_ != STREAM_ID_PRIVATE_STREAM_2 &&
          stream_id_ != STREAM_ID_ECM &&
          stream_id_ != STREAM_ID_EMM &&
          stream_id_ != STREAM_ID_PROGRAM_STREAM_DIRECTORY &&
          stream_id_ != STREAM_ID_ISO13818_1A_DSMCC &&
          stream_id_ != STREAM_ID_ITUT_H222_1E)
      {
        pes_ = PES();

        PES & pes = *pes_;
        pes.load(bin);

        // load data:
        std::size_t end_pos = bin.position();
        std::size_t consumed = end_pos - start_pos;
        YAE_THROW_IF((consumed & 0x7) != 0);

        std::size_t consumed_bytes = consumed >> 3;
        data_ = bin.read_bytes(pes_packet_length_ - consumed_bytes);
      }
      else if (stream_id_ == STREAM_ID_PROGRAM_STREAM_MAP ||
               stream_id_ == STREAM_ID_PRIVATE_STREAM_2 ||
               stream_id_ == STREAM_ID_ECM ||
               stream_id_ == STREAM_ID_EMM ||
               stream_id_ == STREAM_ID_PROGRAM_STREAM_DIRECTORY ||
               stream_id_ == STREAM_ID_ISO13818_1A_DSMCC ||
               stream_id_ == STREAM_ID_ITUT_H222_1E)
      {
        data_ = bin.read_bytes(pes_packet_length_);
      }
      else if (stream_id_ == STREAM_ID_PADDING_STREAM)
      {
        padding_ = bin.read_bytes(pes_packet_length_);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PES::PES
    //
    PESPacket::PES::PES():
      pes_const_10_(0),
      pes_scrambling_control_(0),
      pes_priority_(0),
      data_alignment_indicator_(0),
      copyright_(0),
      original_or_copy_(0),

      pts_dts_flags_(0),
      escr_flag_(0),
      es_rate_flag_(0),
      dsm_trick_mode_flag_(0),
      additional_copy_info_flag_(0),
      pes_crc_flag_(0),
      pes_extension_flag_(0),

      pes_header_data_length_(0),

      pts_prefix_(0),
      pts_32_30_(0),
      pts_marker1_(0),
      pts_29_15_(0),
      pts_marker2_(0),
      pts_14_00_(0),
      pts_marker3_(0),

      dts_prefix_(0),
      dts_32_30_(0),
      dts_marker1_(0),
      dts_29_15_(0),
      dts_marker2_(0),
      dts_14_00_(0),
      dts_marker3_(0),

      escr_reserved_(0),
      escr_base_32_30_(0),
      escr_marker1_(0),
      escr_base_29_15_(0),
      escr_marker2_(0),
      escr_base_14_00_(0),
      escr_marker3_(0),
      escr_extension_(0),
      escr_marker4_(0),

      es_rate_marker1_(0),
      es_rate_(0),
      es_rate_marker2_(0),

      trick_mode_(0),

      additional_copy_marker_(0),
      additional_copy_info_(0),

      previous_pes_packet_crc_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::PES::load
    //
    void
    PESPacket::PES::load(IBitstream & bin)
    {
      pes_const_10_ = bin.read(2);
      YAE_THROW_IF(pes_const_10_ != 2);

      pes_scrambling_control_ = bin.read(2);
      pes_priority_ = bin.read(1);
      data_alignment_indicator_ = bin.read(1);
      copyright_ = bin.read(1);
      original_or_copy_ = bin.read(1);

      pts_dts_flags_ = bin.read(2);
      escr_flag_ = bin.read(1);
      es_rate_flag_ = bin.read(1);
      dsm_trick_mode_flag_ = bin.read(1);
      additional_copy_info_flag_ = bin.read(1);
      pes_crc_flag_ = bin.read(1);
      pes_extension_flag_ = bin.read(1);
      pes_header_data_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      if ((pts_dts_flags_ & 0x2) == 0x2)
      {
        pts_prefix_ = bin.read(4);
        YAE_THROW_IF(pts_prefix_ != pts_dts_flags_);

        pts_32_30_ = bin.read(3);
        pts_marker1_ = bin.read(1);
        YAE_THROW_IF(pts_marker1_ != 1);

        pts_29_15_ = bin.read(15);
        pts_marker2_ = bin.read(1);
        YAE_THROW_IF(pts_marker2_ != 1);

        pts_14_00_ = bin.read(15);
        pts_marker3_ = bin.read(1);
        YAE_THROW_IF(pts_marker3_ != 1);
      }

      if (pts_dts_flags_ == 0x3)
      {
        dts_prefix_ = bin.read(4);
        YAE_THROW_IF(dts_prefix_ != 1);

        dts_32_30_ = bin.read(3);
        dts_marker1_ = bin.read(1);
        YAE_THROW_IF(dts_marker1_ != 1);

        dts_29_15_ = bin.read(15);
        dts_marker2_ = bin.read(1);
        YAE_THROW_IF(dts_marker2_ != 1);

        dts_14_00_ = bin.read(15);
        dts_marker3_ = bin.read(1);
        YAE_THROW_IF(dts_marker3_ != 1);
      }

      if (escr_flag_)
      {
        escr_reserved_ = bin.read(2);
        YAE_THROW_IF(escr_reserved_ != 0x3);

        escr_base_32_30_ = bin.read(3);
        escr_marker1_ = bin.read(1);
        YAE_THROW_IF(escr_marker1_ != 1);

        escr_base_29_15_ = bin.read(15);
        escr_marker2_ = bin.read(1);
        YAE_THROW_IF(escr_marker2_ != 1);

        escr_base_14_00_ = bin.read(15);
        escr_marker3_ = bin.read(1);
        YAE_THROW_IF(escr_marker3_ != 1);

        escr_extension_ = bin.read(9);
        escr_marker4_ = bin.read(1);
        YAE_THROW_IF(escr_marker4_ != 1);
      }

      if (es_rate_flag_)
      {
        es_rate_marker1_ = bin.read(1);
        YAE_THROW_IF(es_rate_marker1_ != 1);

        es_rate_ = bin.read(22);
        es_rate_marker2_ = bin.read(1);
        YAE_THROW_IF(es_rate_marker2_ != 1);
      }

      if (dsm_trick_mode_flag_)
      {
        mode_.trick_mode_control_ = bin.read(3);
        if (mode_.trick_mode_control_ == TRICK_MODE_FAST_FORWARD ||
            mode_.trick_mode_control_ == TRICK_MODE_FAST_REVERSE)
        {
          fast_.field_id_ = bin.read(2);
          fast_.intra_slice_refresh_ = bin.read(1);
          fast_.frequency_truncation_ = bin.read(2);
        }
        else if (mode_.trick_mode_control_ == TRICK_MODE_SLOW_MOTION ||
                 mode_.trick_mode_control_ == TRICK_MODE_SLOW_REVERSE)
        {
          slow_.rep_cntrl_ = bin.read(5);
        }
        else if (mode_.trick_mode_control_ == TRICK_MODE_FREEZE_FRAME)
        {
          freeze_.field_id_ = bin.read(2);
          freeze_.reserved_ = bin.read(3);
          YAE_THROW_IF(freeze_.reserved_ != 0x7);
        }
        else
        {
          mode_.reserved_ = bin.read(5);
          YAE_THROW_IF(mode_.reserved_ != 0x1F);
        }
      }

      if (additional_copy_info_flag_)
      {
        additional_copy_marker_ = bin.read(1);
        YAE_THROW_IF(additional_copy_marker_ != 1);

        additional_copy_info_ = bin.read(7);
      }

      if (pes_crc_flag_)
      {
        previous_pes_packet_crc_ = bin.read(16);
      }

      if (pes_extension_flag_)
      {
        extension_ = Extension();

        Extension & ext = *extension_;
        ext.load(bin);
      }

      // load stuffing:
      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      stuffing_ = bin.read_bytes(pes_header_data_length_ - consumed_bytes);
    }


    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Extension
    //
    PESPacket::PES::Extension::Extension():
      pes_private_data_flag_(0),
      pack_header_field_flag_(0),
      program_packet_sequence_counter_flag_(0),
      pstd_buffer_flag_(0),
      reserved_(0),
      pes_extension_flag_2_(0),
      pack_field_length_(0),
      program_packet_sequence_counter_marker_(0),
      program_packet_sequence_counter_(0),
      mpeg1_mpeg2_identifier_marker_(0),
      mpeg1_mpeg2_identifier_(0),
      original_stuff_length_(0),
      pstd_const_01_(0),
      pstd_buffer_scale_(0),
      pstd_buffer_size_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::PES::Extension::load
    //
    void
    PESPacket::PES::Extension::load(IBitstream & bin)
    {
      pes_private_data_flag_ = bin.read(1);
      pack_header_field_flag_ = bin.read(1);
      program_packet_sequence_counter_flag_ = bin.read(1);
      pstd_buffer_flag_ = bin.read(1);
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      pes_extension_flag_2_ = bin.read(1);

      if (pes_private_data_flag_)
      {
        pes_private_data_ = bin.read_bytes(16);
      }

      if (pack_header_field_flag_)
      {
        pack_field_length_ = bin.read(8);

        pack_header_ = PackHeader();
        PackHeader & pack_header = *pack_header_;
        pack_header.load(bin);
      }

      if (program_packet_sequence_counter_flag_)
      {
        program_packet_sequence_counter_marker_ = bin.read(1);
        YAE_THROW_IF(program_packet_sequence_counter_marker_ != 1);

        program_packet_sequence_counter_ = bin.read(7);
        mpeg1_mpeg2_identifier_marker_ = bin.read(1);
        YAE_THROW_IF(mpeg1_mpeg2_identifier_marker_ != 1);

        mpeg1_mpeg2_identifier_ = bin.read(1);
        original_stuff_length_ = bin.read(6);
      }

      if (pstd_buffer_flag_)
      {
        pstd_const_01_ = bin.read(2);
        YAE_THROW_IF(pstd_const_01_ != 1);

        pstd_buffer_scale_ = bin.read(1);
        pstd_buffer_size_ = bin.read(13);
      }

      if (pes_extension_flag_2_)
      {
        ext2_ = Ext2();
        Ext2 & ext2 = *ext2_;
        ext2.load(bin);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Ext2::Ext2
    //
    PESPacket::PES::Extension::Ext2::Ext2():
      marker_(0),
      pes_extension_field_length_(0),
      tref_reserved_(0),
      tref_32_30_(0),
      tref_marker1_(0),
      tref_29_15_(0),
      tref_marker2_(0),
      tref_14_00_(0),
      tref_marker3_(0)
    {
      stream_id_.extension_flag_ = 0;
      stream_id_.extension_ = 0;
    }

    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Ext2::load
    //
    void
    PESPacket::PES::Extension::Ext2::load(IBitstream & bin)
    {
      marker_ = bin.read(1);
      YAE_THROW_IF(marker_ != 1);
      pes_extension_field_length_ = bin.read(7);

      std::size_t start_pos = bin.position();
      stream_id_.extension_flag_ = bin.read(1);

      if (stream_id_.extension_flag_ == 0)
      {
        stream_id_.extension_ = bin.read(7);
      }
      else
      {
        tref_.stream_id_extension_reserved_ = bin.read(6);
        YAE_THROW_IF(tref_.stream_id_extension_reserved_ != 0x3F);

        tref_.extension_flag_ = bin.read(1);
        if (tref_.extension_flag_)
        {
          tref_reserved_ = bin.read(4);
          YAE_THROW_IF(tref_reserved_ != 0xF);

          tref_32_30_ = bin.read(3);
          tref_marker1_ = bin.read(1);
          YAE_THROW_IF(tref_marker1_ != 1);

          tref_29_15_ = bin.read(15);
          tref_marker2_ = bin.read(1);
          YAE_THROW_IF(tref_marker2_ != 1);

          tref_14_00_ = bin.read(15);
          tref_marker3_ = bin.read(1);
          YAE_THROW_IF(tref_marker3_ != 1);
        }
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      reserved_ = bin.read_bytes(pes_extension_field_length_ -
                                 consumed_bytes);
    }


    //----------------------------------------------------------------
    // MultipleStringStructure::MultipleStringStructure
    //
    MultipleStringStructure::MultipleStringStructure():
      number_strings_(0)
    {}

    //----------------------------------------------------------------
    // MultipleStringStructure::load
    //
    void
    MultipleStringStructure::load(IBitstream & bin)
    {
      number_strings_ = bin.read(8);

      strings_.resize(number_strings_);
      for (std::size_t i = 0; i < number_strings_; i++)
      {
        Message & message = strings_[i];
        message.load(bin);
      }
    }

    //----------------------------------------------------------------
    // MultipleStringStructure::to_str
    //
    std::string
    MultipleStringStructure::to_str() const
    {
      std::ostringstream oss;
      for (std::size_t i = 0, n = strings_.size(); i < n; i++)
      {
        const Message & msg = strings_[i];
        oss << msg.to_str() << std::endl;
      }

      return oss.str();
    }

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::Message
    //
    MultipleStringStructure::Message::Message():
      number_segments_(0)
    {
      memset(iso_639_language_code_, 0, sizeof(iso_639_language_code_));
    }

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::load
    //
    void
    MultipleStringStructure::Message::load(IBitstream & bin)
    {
      bin.read_bytes(iso_639_language_code_, 3);
      number_segments_ = bin.read(8);

      segment_.resize(number_segments_);
      for (std::size_t i = 0; i < number_segments_; i++)
      {
        Segment & segment = segment_[i];
        segment.load(bin);
      }
    }

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::to_str
    //
    std::string
    MultipleStringStructure::Message::to_str() const
    {
      std::ostringstream oss;
      oss << char(iso_639_language_code_[0])
          << char(iso_639_language_code_[1])
          << char(iso_639_language_code_[2]);

      for (std::size_t i = 0, n = segment_.size(); i < n; i++)
      {
        const MultipleStringStructure::Message::Segment & s = segment_[i];

        std::string text;
        if (s.to_str(text))
        {
          oss << ": " << text;
        }
      }

      return oss.str();
   }

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::Segment::Segment
    //
    MultipleStringStructure::Message::Segment::Segment():
      compression_type_(0),
      mode_(0),
      number_bytes_(0)
    {}

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::Segment::load
    //
    void
    MultipleStringStructure::Message::Segment::load(IBitstream & bin)
    {
      compression_type_ = bin.read(8);
      mode_ = bin.read(8);
      number_bytes_ = bin.read(8);
      compressed_string_ = bin.read_bytes(number_bytes_);
    }

    //----------------------------------------------------------------
    // MultipleStringStructure::Message::Segment::to_str
    //
    bool
    MultipleStringStructure::Message::Segment::to_str(std::string & text) const
    {
      YAE_EXPECT(compression_type_ == 0x00);
      YAE_EXPECT(mode_ != 0x3E);

      if (compression_type_ != NO_COMPRESSION)
      {
        // not supported it at this time: compression
        return false;
      }

      const IBuffer & data = *(compressed_string_);
      const uint8_t * src = data.get();
      const uint8_t * end = data.end();

      if ((mode_ < 0x07) ||
          (0x09 <= mode_ && mode_ < 0x11) ||
          (0x20 <= mode_ && mode_ < 0x28) ||
          (0x30 <= mode_ && mode_ < 0x34))
      {
        for (; src < end; ++src)
        {
          uint32_t uc = (uint32_t(mode_) << 8) | uint32_t(*src);
          yae::unicode_to_utf8(uc, text);
        }

        return true;
      }
      else if (mode_ == 0x3E)
      {
        // not supported at this time:
        // Standard Compression Scheme for Unicode (SCSU)
        return false;
      }
      else if (mode_ == 0x3F)
      {
        // UTF-16
        yae::utf16be_to_utf8(src, end, text);
        return true;
      }

      return false;
    }

    //----------------------------------------------------------------
    // to_str
    //
    std::string
    to_str(const MultipleStringStructure & mss)
    {
      return mss.to_str();
    }


    //----------------------------------------------------------------
    // Descriptor::Descriptor
    //
    Descriptor::Descriptor():
      descriptor_tag_(0),
      descriptor_length_(0)
    {}

    //----------------------------------------------------------------
    // Descriptor::~Descriptor
    //
    Descriptor::~Descriptor()
    {}

    //----------------------------------------------------------------
    // Descriptor::load_header
    //
    void
    Descriptor::load_header(IBitstream & bin)
    {
      descriptor_tag_ = bin.read(8);
      descriptor_length_ = bin.read(8);
    }

    //----------------------------------------------------------------
    // Descriptor::load_body
    //
    void
    Descriptor::load_body(IBitstream & bin)
    {
      bin.skip_bytes(descriptor_length_);
    }

    //----------------------------------------------------------------
    // Descriptor::load
    //
    void
    Descriptor::load(IBitstream & bin)
    {
      this->load_header(bin);
      this->load_body(bin);
    }


    //----------------------------------------------------------------
    // RawDescriptor::load_body
    //
    void
    RawDescriptor::load_body(IBitstream & bin)
    {
      payload_ = bin.read_bytes(descriptor_length_);
    }


    //----------------------------------------------------------------
    // VideoStreamDescriptor::VideoStreamDescriptor
    //
    VideoStreamDescriptor::VideoStreamDescriptor():
      multiple_frame_rate_flag_(0),
      frame_rate_code_(0),
      mpeg1_only_flag_(0),
      constrained_parameter_flag_(0),
      still_picture_flag_(0),
      profile_and_level_indication_(0),
      chroma_format_(0),
      frame_rate_extension_flag_(0),
      reserved_(0)
    {}

    //----------------------------------------------------------------
    // VideoStreamDescriptor::load_body
    //
    void
    VideoStreamDescriptor::load_body(IBitstream & bin)
    {
      multiple_frame_rate_flag_ = bin.read(1);
      frame_rate_code_ = bin.read(4);
      mpeg1_only_flag_ = bin.read(1);
      constrained_parameter_flag_ = bin.read(1);
      still_picture_flag_ = bin.read(1);
      if (!mpeg1_only_flag_)
      {
        profile_and_level_indication_ = bin.read(8);
        chroma_format_ = bin.read(2);
        frame_rate_extension_flag_ = bin.read(1);
        reserved_ = bin.read(5);
        // YAE_THROW_IF(reserved_ != 0x1F);
      }
    }


    //----------------------------------------------------------------
    // AudioStreamDescriptor::AudioStreamDescriptor
    //
    AudioStreamDescriptor::AudioStreamDescriptor():
      free_format_flag_(0),
      id_(0),
      layer_(0),
      variable_rate_audio_indicator_(0),
      reserved_(0)
    {}

    //----------------------------------------------------------------
    // AudioStreamDescriptor::load_body
    //
    void
    AudioStreamDescriptor::load_body(IBitstream & bin)
    {
      free_format_flag_ = bin.read(1);
      id_ = bin.read(1);
      layer_ = bin.read(2);
      variable_rate_audio_indicator_ = bin.read(1);
      reserved_ = bin.read(3);
    }


    //----------------------------------------------------------------
    // HierarchyDescriptor::HierarchyDescriptor
    //
    HierarchyDescriptor::HierarchyDescriptor():
      reserved1_(0),
      temporal_scalability_flag_(0),
      spatial_scalability_flag_(0),
      quality_scalability_flag_(0),
      hierarchy_type_(0),
      reserved2_(0),
      hierarchy_layer_index_(0),
      tref_present_flag_(0),
      reserved3_(0),
      hierarchy_embedded_layer_index_(0),
      reserved4_(0),
      hierarchy_channel_(0)
    {}

    //----------------------------------------------------------------
    // HierarchyDescriptor::load_body
    //
    void
    HierarchyDescriptor::load_body(IBitstream & bin)
    {
      reserved1_ = bin.read(1);
      temporal_scalability_flag_ = bin.read(1);
      spatial_scalability_flag_ = bin.read(1);
      quality_scalability_flag_ = bin.read(1);
      hierarchy_type_ = bin.read(4);
      reserved2_ = bin.read(2);
      hierarchy_layer_index_ = bin.read(6);
      tref_present_flag_ = bin.read(1);
      reserved3_ = bin.read(1);
      hierarchy_embedded_layer_index_ = bin.read(6);
      reserved4_ = bin.read(2);
      hierarchy_channel_ = bin.read(6);
    }


    //----------------------------------------------------------------
    // RegistrationDescriptor::RegistrationDescriptor
    //
    RegistrationDescriptor::RegistrationDescriptor():
      format_identifier_(0)
    {}

    //----------------------------------------------------------------
    // RegistrationDescriptor::load_body
    //
    void
    RegistrationDescriptor::load_body(IBitstream & bin)
    {
      format_identifier_ = bin.read(32);
      additional_identification_info_ = bin.read_bytes(descriptor_length_ - 4);
    }


    //----------------------------------------------------------------
    // DataStreamAlignmentDescriptor::DataStreamAlignmentDescriptor
    //
    DataStreamAlignmentDescriptor::DataStreamAlignmentDescriptor():
      alignment_type_(0)
    {}

    //----------------------------------------------------------------
    // DataStreamAlignmentDescriptor::load_body
    //
    void
    DataStreamAlignmentDescriptor::load_body(IBitstream & bin)
    {
      alignment_type_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // TargetBackgroundGridDescriptor::TargetBackgroundGridDescriptor
    //
    TargetBackgroundGridDescriptor::TargetBackgroundGridDescriptor():
      horizontal_size_(0),
      vertical_size_(0),
      aspect_ratio_information_(0)
    {}

    //----------------------------------------------------------------
    // TargetBackgroundGridDescriptor::load_body
    //
    void
    TargetBackgroundGridDescriptor::load_body(IBitstream & bin)
    {
      horizontal_size_ = bin.read(14);
      vertical_size_ = bin.read(14);
      aspect_ratio_information_ = bin.read(4);
    }


    //----------------------------------------------------------------
    // VideoWindowDescriptor::VideoWindowDescriptor
    //
    VideoWindowDescriptor::VideoWindowDescriptor():
      horizontal_offset_(0),
      vertical_offset_(0),
      window_priority_(0)
    {}

    //----------------------------------------------------------------
    // VideoWindowDescriptor::load_body
    //
    void
    VideoWindowDescriptor::load_body(IBitstream & bin)
    {
      horizontal_offset_ = bin.read(14);
      vertical_offset_ = bin.read(14);
      window_priority_ = bin.read(4);
    }


    //----------------------------------------------------------------
    // CADescriptor::CADescriptor
    //
    CADescriptor::CADescriptor():
      ca_system_id_(0),
      reserved_(0),
      ca_pid_(0)
    {}

    //----------------------------------------------------------------
    // CADescriptor::load_body
    //
    void
    CADescriptor::load_body(IBitstream & bin)
    {
      ca_system_id_ = bin.read(16);
      reserved_ = bin.read(3);
      ca_pid_ = bin.read(13);
      private_data_ = bin.read_bytes(descriptor_length_ - 4);
    }


    //----------------------------------------------------------------
    // ISO639LanguageDescriptor::load_body
    //
    void
    ISO639LanguageDescriptor::load_body(IBitstream & bin)
    {
      std::size_t n = descriptor_length_ / 4;
      YAE_THROW_IF((descriptor_length_ & 0x3) != 0x0);

      lang_.resize(n);
      for (std::size_t i = 0; i < n; i++)
      {
        Lang & lang = lang_[i];
        lang.load(bin);
      }
    }

    //----------------------------------------------------------------
    // ISO639LanguageDescriptor::Lang::Lang
    //
    ISO639LanguageDescriptor::Lang::Lang():
      audio_type_(0)
    {
      memset(iso_639_language_code_, 0, sizeof(iso_639_language_code_));
    }

    //----------------------------------------------------------------
    // ISO639LanguageDescriptor::Lang::load
    //
    void
    ISO639LanguageDescriptor::Lang::load(IBitstream & bin)
    {
      bin.read_bytes(iso_639_language_code_, 3);
      audio_type_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // SystemClockDescriptor::SystemClockDescriptor
    //
    SystemClockDescriptor::SystemClockDescriptor():
      external_clock_reference_indicator_(0),
      reserved1_(0),
      clock_accuracy_integer_(0),
      clock_accuracy_exponent_(0),
      reserved2_(0)
    {}

    //----------------------------------------------------------------
    // SystemClockDescriptor::load_body
    //
    void
    SystemClockDescriptor::load_body(IBitstream & bin)
    {
      external_clock_reference_indicator_ = bin.read(1);
      reserved1_ = bin.read(1);
      clock_accuracy_integer_ = bin.read(6);
      clock_accuracy_exponent_ = bin.read(3);
      reserved2_ = bin.read(5);
    }


    //----------------------------------------------------------------
    // MultiplexBufferUtilizationDescriptor::
    // MultiplexBufferUtilizationDescriptor
    //
    MultiplexBufferUtilizationDescriptor::
    MultiplexBufferUtilizationDescriptor():
      bound_valid_flag_(0),
      ltw_offset_lower_bound_(0),
      reserved_(0),
      ltw_offset_upper_bound_(0)
    {}

    //----------------------------------------------------------------
    // MultiplexBufferUtilizationDescriptor::load_body
    //
    void
    MultiplexBufferUtilizationDescriptor::load_body(IBitstream & bin)
    {
      bound_valid_flag_ = bin.read(1);
      ltw_offset_lower_bound_ = bin.read(15);
      reserved_ = bin.read(1);
      ltw_offset_upper_bound_ = bin.read(15);
    }


    //----------------------------------------------------------------
    // CopyrightDescriptor::CopyrightDescriptor
    //
    CopyrightDescriptor::CopyrightDescriptor():
      copyright_identifier_(0)
    {}

    //----------------------------------------------------------------
    // CopyrightDescriptor::load_body
    //
    void
    CopyrightDescriptor::load_body(IBitstream & bin)
    {
      copyright_identifier_ = bin.read(32);
      additional_copyright_info_ = bin.read_bytes(descriptor_length_ - 4);
    }


    //----------------------------------------------------------------
    // MaximumBitrateDescriptor::MaximumBitrateDescriptor
    //
    MaximumBitrateDescriptor::MaximumBitrateDescriptor():
      reserved_(0),
      maximum_bitrate_(0)
    {}

    //----------------------------------------------------------------
    // MaximumBitrateDescriptor::load_body
    //
    void
    MaximumBitrateDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(2);
      maximum_bitrate_ = bin.read(22);
    }


    //----------------------------------------------------------------
    // PrivateDataIndicatorDescriptor::PrivateDataIndicatorDescriptor
    //
    PrivateDataIndicatorDescriptor::PrivateDataIndicatorDescriptor():
      private_data_indicator_(0)
    {}

    //----------------------------------------------------------------
    // PrivateDataIndicatorDescriptor::load_body
    //
    void
    PrivateDataIndicatorDescriptor::load_body(IBitstream & bin)
    {
      private_data_indicator_ = bin.read(32);
    }


    //----------------------------------------------------------------
    // SmoothingBufferDescriptor::SmoothingBufferDescriptor
    //
    SmoothingBufferDescriptor::SmoothingBufferDescriptor():
      reserved1_(0),
      sb_leak_rate_(0),
      reserved2_(0),
      sb_size_(0)
    {}

    //----------------------------------------------------------------
    // SmoothingBufferDescriptor::load_body
    //
    void
    SmoothingBufferDescriptor::load_body(IBitstream & bin)
    {
      reserved1_ = bin.read(2);
      sb_leak_rate_ = bin.read(22);
      reserved2_ = bin.read(2);
      sb_size_ = bin.read(22);
    }


    //----------------------------------------------------------------
    // STDDescriptor::STDDescriptor
    //
    STDDescriptor::STDDescriptor():
      reserved_(0),
      leak_valid_flag_(0)
    {}

    //----------------------------------------------------------------
    // STDDescriptor::load_body
    //
    void
    STDDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(7);
      leak_valid_flag_ = bin.read(1);
    }


    //----------------------------------------------------------------
    // IBPDescriptor::IBPDescriptor
    //
    IBPDescriptor::IBPDescriptor():
      closed_gop_flag_(0),
      identical_gop_flag_(0),
      max_gop_length_(0)
    {}

    //----------------------------------------------------------------
    // IBPDescriptor::load_body
    //
    void
    IBPDescriptor::load_body(IBitstream & bin)
    {
      closed_gop_flag_ = bin.read(1);
      identical_gop_flag_ = bin.read(1);
      max_gop_length_ = bin.read(14);
    }


    //----------------------------------------------------------------
    // MPEG4VideoDescriptor::MPEG4VideoDescriptor
    //
    MPEG4VideoDescriptor::MPEG4VideoDescriptor():
      mpeg4_visual_profile_and_level_(0)
    {}

    //----------------------------------------------------------------
    // MPEG4VideoDescriptor::load_body
    //
    void
    MPEG4VideoDescriptor::load_body(IBitstream & bin)
    {
      mpeg4_visual_profile_and_level_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // MPEG4AudioDescriptor::MPEG4AudioDescriptor
    //
    MPEG4AudioDescriptor::MPEG4AudioDescriptor():
      mpeg4_audio_profile_and_level_(0)
    {}

    //----------------------------------------------------------------
    // MPEG4AudioDescriptor::load_body
    //
    void
    MPEG4AudioDescriptor::load_body(IBitstream & bin)
    {
      mpeg4_audio_profile_and_level_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // IODDescriptor::IODDescriptor
    //
    IODDescriptor::IODDescriptor():
      scope_of_iod_label_(0),
      iod_label_(0)
    {}

    //----------------------------------------------------------------
    // IODDescriptor::load_body
    //
    void
    IODDescriptor::load_body(IBitstream & bin)
    {
      scope_of_iod_label_ = bin.read(8);
      iod_label_ = bin.read(8);
      initial_object_descriptor_ = bin.read_bytes(descriptor_length_ - 2);
    }


    //----------------------------------------------------------------
    // SLDescriptor::SLDescriptor
    //
    SLDescriptor::SLDescriptor():
      es_id_(0)
    {}

    //----------------------------------------------------------------
    // SLDescriptor::load_body
    //
    void
    SLDescriptor::load_body(IBitstream & bin)
    {
      es_id_ = bin.read(16);
    }


    //----------------------------------------------------------------
    // FMCDescriptor::load_body
    //
    void FMCDescriptor::load_body(IBitstream & bin)
    {
      std::size_t stop_pos = bin.position_plus_nbytes(descriptor_length_);
      while (bin.position() < stop_pos)
      {
        flex_mux_.push_back(FlexMux());
        FlexMux & flex_mux = flex_mux_.back();
        flex_mux.load(bin);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }

    //----------------------------------------------------------------
    // FMCDescriptor::FlexMux::FlexMux
    //
    FMCDescriptor::FlexMux::FlexMux():
      es_id_(0),
      flex_mux_channel_(0)
    {}

    //----------------------------------------------------------------
    // FMCDescriptor::FlexMux::load
    //
    void
    FMCDescriptor::FlexMux::load(IBitstream & bin)
    {
      es_id_ = bin.read(16);
      flex_mux_channel_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // ExternalESIDDescriptor::ExternalESIDDescriptor
    //
    ExternalESIDDescriptor::ExternalESIDDescriptor():
      external_es_id_(0)
    {}

    //----------------------------------------------------------------
    // ExternalESIDDescriptor::load_body
    //
    void
    ExternalESIDDescriptor::load_body(IBitstream & bin)
    {
      external_es_id_ = bin.read(16);
    }


    //----------------------------------------------------------------
    // MuxcodeDescriptor::load_body
    //
    void
    MuxcodeDescriptor::load_body(IBitstream & bin)
    {
      mux_code_table_entries_ = bin.read_bytes(descriptor_length_);
    }


    //----------------------------------------------------------------
    // MultiplexBufferDescriptor::MultiplexBufferDescriptor
    //
    MultiplexBufferDescriptor::MultiplexBufferDescriptor():
      mb_buffer_size_(0),
      tb_leak_rate_(0)
    {}

    //----------------------------------------------------------------
    // MultiplexBufferDescriptor::load_body
    //
    void
    MultiplexBufferDescriptor::load_body(IBitstream & bin)
    {
      mb_buffer_size_ = bin.read(24);
      tb_leak_rate_ = bin.read(24);
    }


    //----------------------------------------------------------------
    // FlexMuxTimingDescriptor::FlexMuxTimingDescriptor
    //
    FlexMuxTimingDescriptor::FlexMuxTimingDescriptor():
      fcr_es_id_(0),
      fcr_resolution_(0),
      fcr_length_(0),
      fmx_rate_length_(0)
    {}

    //----------------------------------------------------------------
    // FlexMuxTimingDescriptor::load_body
    //
    void
    FlexMuxTimingDescriptor::load_body(IBitstream & bin)
    {
      fcr_es_id_ = bin.read(16);
      fcr_resolution_ = bin.read(32);
      fcr_length_ = bin.read(8);
      fmx_rate_length_ = bin.read(8);
    }


    //----------------------------------------------------------------
    // MPEG2StereoscopicVideoFormatDescriptor::
    //
    MPEG2StereoscopicVideoFormatDescriptor::
    MPEG2StereoscopicVideoFormatDescriptor():
      stereoscopic_video_arrangement_type_present_(0),
      stereoscopic_video_arrangement_type_(0)
    {}

    //----------------------------------------------------------------
    // MPEG2StereoscopicVideoFormatDescriptor::load_body
    //
    void
    MPEG2StereoscopicVideoFormatDescriptor::load_body(IBitstream & bin)
    {
      stereoscopic_video_arrangement_type_present_ = bin.read(1);
      stereoscopic_video_arrangement_type_ = bin.read(7);
    }


    //----------------------------------------------------------------
    // AC3AudioDescriptor::AC3AudioDescriptor
    //
    AC3AudioDescriptor::AC3AudioDescriptor():
      sample_rate_code_(0),
      bsid_(0),
      bit_rate_code_(0),
      surround_mode_(0),
      bsmod_(0),
      num_channels_(0),
      full_svc_(0),
      langcod_(0),
      langcod2_(0),
      asvcflags_(0),
      textlen_(0),
      text_code_(0),
      language_flag_(0),
      language2_flag_(0),
      reserved2_(0)
    {
      memset(language_, 0, sizeof(language_));
      memset(language2_, 0, sizeof(language2_));
    }

    //----------------------------------------------------------------
    // AC3AudioDescriptor::load_body
    //
    void
    AC3AudioDescriptor::load_body(IBitstream & bin)
    {
      std::size_t start_pos = bin.position();

      sample_rate_code_ = bin.read(3);
      bsid_ = bin.read(5);
      bit_rate_code_ = bin.read(6);
      surround_mode_ = bin.read(2);
      bsmod_ = bin.read(3);
      num_channels_ = bin.read(4);
      full_svc_ = bin.read(1);

      langcod_ = bin.read(8);
      if (!num_channels_)
      {
        langcod2_ = bin.read(8);
      }

      asvcflags_ = bin.read(8);

      textlen_ = bin.read(7);
      text_code_ = bin.read(1);
      text_ = bin.read_bytes(textlen_);

      language_flag_  = bin.read(1);
      language2_flag_ = bin.read(1);
      reserved2_ = bin.read(6);
      // YAE_THROW_IF(reserved2_ != 0x3F);

      if (language_flag_)
      {
        bin.read_bytes(language_, 3);
      }

      if (language2_flag_)
      {
        bin.read_bytes(language2_, 3);
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      additional_info_ = bin.read_bytes(descriptor_length_ - consumed_bytes);
    }


    //----------------------------------------------------------------
    // CaptionServiceDescriptor::CaptionServiceDescriptor
    //
    CaptionServiceDescriptor::CaptionServiceDescriptor():
      reserved_(0),
      number_of_services_(0)
    {}

    //----------------------------------------------------------------
    // CaptionServiceDescriptor::load_body
    //
    void
    CaptionServiceDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      number_of_services_ = bin.read(5);
      service_.resize(number_of_services_);

      for (std::size_t i = 0; i < number_of_services_; i++)
      {
        Service & service = service_[i];
        service.load(bin);
      }
    }

    //----------------------------------------------------------------
    // CaptionServiceDescriptor::Service::Service
    //
    CaptionServiceDescriptor::Service::Service():
      digital_cc_(0),
      reserved1_(0),
      caption_service_number_(0),
      easy_reader_(0),
      wide_aspect_ratio_(0),
      reserved3_(0)
    {
      memset(language_, 0, sizeof(language_));
    }

    //----------------------------------------------------------------
    // CaptionServiceDescriptor::Service::load
    //
    void
    CaptionServiceDescriptor::Service::load(IBitstream & bin)
    {
      bin.read_bytes(language_, 3);
      digital_cc_ = bin.read(1);
      reserved1_ = bin.read(1);
      YAE_THROW_IF(reserved1_ != 1);

      if (digital_cc_)
      {
        caption_service_number_ = bin.read(6);
      }
      else
      {
        reserved2_ = bin.read(5);
        YAE_THROW_IF(reserved2_ != 0x1F);
        line21_field_ = bin.read(1);
      }

      easy_reader_ = bin.read(1);
      wide_aspect_ratio_ = bin.read(1);
      reserved3_ = bin.read(14);
      YAE_THROW_IF(reserved3_ != 0x3FFF);
    }


    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::ContentAdvisoryDescriptor
    //
    ContentAdvisoryDescriptor::ContentAdvisoryDescriptor():
      reserved_(0),
      rating_region_count_(0)
    {}

    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::load_body
    //
    void
    ContentAdvisoryDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(2);
      rating_region_count_ = bin.read(6);

      region_.resize(rating_region_count_);
      for (std::size_t i = 0; i < rating_region_count_; i++)
      {
        Region & region = region_[i];
        region.load(bin);
      }
    }

    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::Region::Region
    //
    ContentAdvisoryDescriptor::Region::Region():
      rating_region_(0),
      rated_dimensions_(0),
      rating_description_length_(0)
    {}

    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::Region::load
    //
    void
    ContentAdvisoryDescriptor::Region::load(IBitstream & bin)
    {
      rating_region_ = bin.read(8);
      rated_dimensions_ = bin.read(8);

      dimension_.resize(rated_dimensions_);
      for (std::size_t i = 0; i < rated_dimensions_; i++)
      {
        Dimension & dimension = dimension_[i];
        dimension.load(bin);
      }

      rating_description_length_ = bin.read(8);
      std::size_t stop_pos =
        bin.position_plus_nbytes(rating_description_length_);
      rating_description_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "ContentAdvisoryDescriptor::Region, rating: "
        << rating_description_text_.to_str()
        << std::endl;
    }

    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::Region::Dimension::Dimension
    //
    ContentAdvisoryDescriptor::Region::Dimension::Dimension():
      rating_dimension_(0),
      reserved_(0),
      rating_value_(0)
    {}

    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor::Region::Dimension::load
    //
    void
    ContentAdvisoryDescriptor::Region::Dimension::load(IBitstream & bin)
    {
      rating_dimension_ = bin.read(8);
      reserved_ = bin.read(4);
      YAE_THROW_IF(reserved_ != 0xF);
      rating_value_ = bin.read(4);
    }


    //----------------------------------------------------------------
    // ExtendedChannelNameDescriptor::load_body
    //
    void
    ExtendedChannelNameDescriptor::load_body(IBitstream & bin)
    {
      long_channel_name_text_.load(bin);

      // FIXME: pkoshevoy:
      std::cerr
        << "ExtendedChannelNameDescriptor: "
        << long_channel_name_text_.to_str()
        << std::endl;
    }


    //----------------------------------------------------------------
    // ServiceLocationDescriptor::ServiceLocationDescriptor
    //
    ServiceLocationDescriptor::ServiceLocationDescriptor():
      reserved_(0),
      pcr_pid_(0),
      number_elements_(0)
    {}

    //----------------------------------------------------------------
    // ServiceLocationDescriptor::load_body
    //
    void
    ServiceLocationDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      pcr_pid_ = bin.read(13);
      number_elements_ = bin.read(8);

      element_.resize(number_elements_);
      for (std::size_t i = 0; i < number_elements_; i++)
      {
        Element & element = element_[i];
        element.load(bin);
      }
    }

    //----------------------------------------------------------------
    // ServiceLocationDescriptor::Element::Element
    //
    ServiceLocationDescriptor::Element::Element():
      stream_type_(0),
      reserved_(0),
      elementary_pid_(0)
    {
      memset(iso_639_languace_code_, 0, sizeof(iso_639_languace_code_));
    }

    //----------------------------------------------------------------
    // ServiceLocationDescriptor::Element::load
    //
    void
    ServiceLocationDescriptor::Element::load(IBitstream & bin)
    {
      stream_type_ = bin.read(8);
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      elementary_pid_ = bin.read(13);
      bin.read_bytes(iso_639_languace_code_, 3);
    }


    //----------------------------------------------------------------
    // TimeShiftedServiceDescriptor::TimeShiftedServiceDescriptor
    //
    TimeShiftedServiceDescriptor::TimeShiftedServiceDescriptor():
      reserved_(0),
      number_of_services_(0)
    {}

    //----------------------------------------------------------------
    // TimeShiftedServiceDescriptor::load_body
    //
    void
    TimeShiftedServiceDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      number_of_services_ = bin.read(5);
      service_.resize(number_of_services_);

      for (std::size_t i = 0; i < number_of_services_; i++)
      {
        Service & service = service_[i];
        service.load(bin);
      }
    }

    //----------------------------------------------------------------
    // TimeShiftedServiceDescriptor::Service::Service
    //
    TimeShiftedServiceDescriptor::Service::Service():
      reserved1_(0),
      time_shift_(0),
      reserved2_(0),
      major_channel_number_(0),
      minor_channel_number_(0)
    {}

    //----------------------------------------------------------------
    // TimeShiftedServiceDescriptor::Service::load
    //
    void
    TimeShiftedServiceDescriptor::Service::load(IBitstream & bin)
    {
      reserved1_ = bin.read(6);
      YAE_THROW_IF(reserved1_ != 0x3F);

      time_shift_ = bin.read(10);
      reserved2_ = bin.read(4);
      YAE_THROW_IF(reserved2_ != 0xF);

      major_channel_number_ = bin.read(10);
      minor_channel_number_ = bin.read(10);
    }


    //----------------------------------------------------------------
    // ComponentNameDescriptor::load_body
    //
    void
    ComponentNameDescriptor::load_body(IBitstream & bin)
    {
      component_name_string_.load(bin);

      // FIXME: pkoshevoy:
      std::cerr
        << "ComponentNameDescriptor: "
        << component_name_string_.to_str()
        << std::endl;
    }


    //----------------------------------------------------------------
    // DCCRequestDescriptor::DCCRequestDescriptor
    //
    DCCRequestDescriptor::DCCRequestDescriptor():
      dcc_request_type_(0),
      dcc_request_text_length_(0)
    {}

    //----------------------------------------------------------------
    // DCCRequestDescriptor::load_body
    //
    void
    DCCRequestDescriptor::load_body(IBitstream & bin)
    {
      dcc_request_type_ = bin.read(8);
      dcc_request_text_length_ = bin.read(8);

      std::size_t start_pos = bin.position();
      dcc_request_text_.load(bin);

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      bin.skip_bytes(dcc_request_text_length_ - consumed_bytes);

      // FIXME: pkoshevoy:
      std::cerr
        << "DCCRequestDescriptor: "
        << dcc_request_text_.to_str()
        << std::endl;
    }


    //----------------------------------------------------------------
    // RedistributionControlDescriptor::load_body
    //
    void
    RedistributionControlDescriptor::load_body(IBitstream & bin)
    {
      rc_information_ = bin.read_bytes(descriptor_length_);
    }


    //----------------------------------------------------------------
    // GenreDescriptor::GenreDescriptor
    //
    GenreDescriptor::GenreDescriptor():
      reserved_(0),
      attribute_count_(0)
    {}

    //----------------------------------------------------------------
    // GenreDescriptor::load_body
    //
    void
    GenreDescriptor::load_body(IBitstream & bin)
    {
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      attribute_count_  = bin.read(5);
      attribute_ = bin.read_bytes(attribute_count_);
    }


    //----------------------------------------------------------------
    // EAC3AudioStreamDescriptor::EAC3AudioStreamDescriptor
    //
    EAC3AudioStreamDescriptor::EAC3AudioStreamDescriptor():
      reserved1_(0),
      bsid_flag_(0),
      mainid_flag_(0),
      asvc_flag_(0),
      mixinfoexists_(0),
      substream1_flag_(0),
      substream2_flag_(0),
      substream3_flag_(0),
      reserved2_(0),
      full_service_flag_(0),
      audio_service_type_(0),
      number_of_channels_(0),

      language_flag_(0),
      language2_flag_(0),
      reserved3_(0),
      bsid_(0),

      reserved4_(0),
      priority_(0),
      mainid_(0),

      asvc_(0),
      substream1_(0),
      substream2_(0),
      substream3_(0)
    {
      memcpy(language_, 0, sizeof(language_));
      memcpy(language2_, 0, sizeof(language2_));

      memcpy(substream1_lang_, 0, sizeof(substream1_lang_));
      memcpy(substream2_lang_, 0, sizeof(substream2_lang_));
      memcpy(substream3_lang_, 0, sizeof(substream3_lang_));
    }

    //----------------------------------------------------------------
    // EAC3AudioStreamDescriptor::load_body
    //
    void
    EAC3AudioStreamDescriptor::load_body(IBitstream & bin)
    {
      std::size_t start_pos = bin.position();
      reserved1_ = bin.read(1);
      YAE_THROW_IF(reserved1_ != 0x1);

      bsid_flag_ = bin.read(1);
      mainid_flag_ = bin.read(1);
      asvc_flag_ = bin.read(1);
      mixinfoexists_ = bin.read(1);
      substream1_flag_ = bin.read(1);
      substream2_flag_ = bin.read(1);
      substream3_flag_ = bin.read(1);
      reserved2_ = bin.read(1);
      YAE_THROW_IF(reserved2_ != 0x1);

      full_service_flag_ = bin.read(1);
      audio_service_type_ = bin.read(3);
      number_of_channels_ = bin.read(3);

      language_flag_ = bin.read(1);
      language2_flag_ = bin.read(1);
      reserved3_ = bin.read(1);
      YAE_THROW_IF(reserved3_ != 0x1);
      bsid_ = bin.read(5);

      if (mainid_flag_)
      {
        reserved4_ = bin.read(3);
        YAE_THROW_IF(reserved4_ != 0x7);

        priority_ = bin.read(2);
        mainid_ = bin.read(3);
      }

      if (asvc_flag_)
      {
        asvc_ = bin.read(8);
      }

      if (substream1_flag_)
      {
        substream1_ = bin.read(8);
      }

      if (substream2_flag_)
      {
        substream2_ = bin.read(8);
      }

      if (substream3_flag_)
      {
        substream3_ = bin.read(8);
      }

      if (language_flag_)
      {
        bin.read_bytes(language_, 3);
      }

      if (language2_flag_)
      {
        bin.read_bytes(language2_, 3);
      }

      if (substream1_flag_)
      {
        bin.read_bytes(substream1_lang_, 3);
      }

      if (substream2_flag_)
      {
        bin.read_bytes(substream2_lang_, 3);
      }

      if (substream3_flag_)
      {
        bin.read_bytes(substream3_lang_, 3);
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      additional_info_ = bin.read_bytes(descriptor_length_ - consumed_bytes);
    }


    //----------------------------------------------------------------
    // load_descriptor
    //
    TDescriptorPtr
    load_descriptor(IBitstream & bin)
    {
      TDescriptorPtr descriptor;
      uint8_t descriptor_tag = bin.peek(8);

      if (descriptor_tag == 0x02)
      {
        descriptor.reset(new VideoStreamDescriptor());
      }
      else if (descriptor_tag == 0x03)
      {
        descriptor.reset(new AudioStreamDescriptor());
      }
      else if (descriptor_tag == 0x04)
      {
        descriptor.reset(new HierarchyDescriptor());
      }
      else if (descriptor_tag == 0x05)
      {
        descriptor.reset(new RegistrationDescriptor());
      }
      else if (descriptor_tag == 0x06)
      {
        descriptor.reset(new DataStreamAlignmentDescriptor());
      }
      else if (descriptor_tag == 0x07)
      {
        descriptor.reset(new TargetBackgroundGridDescriptor());
      }
      else if (descriptor_tag == 0x08)
      {
        descriptor.reset(new VideoWindowDescriptor());
      }
      else if (descriptor_tag == 0x09)
      {
        descriptor.reset(new CADescriptor());
      }
      else if (descriptor_tag == 0x0A)
      {
        descriptor.reset(new ISO639LanguageDescriptor());
      }
      else if (descriptor_tag == 0x0B)
      {
        descriptor.reset(new SystemClockDescriptor());
      }
      else if (descriptor_tag == 0x0C)
      {
        descriptor.reset(new MultiplexBufferUtilizationDescriptor());
      }
      else if (descriptor_tag == 0x0D)
      {
        descriptor.reset(new CopyrightDescriptor());
      }
      else if (descriptor_tag == 0x0E)
      {
        descriptor.reset(new MaximumBitrateDescriptor());
      }
      else if (descriptor_tag == 0x0F)
      {
        descriptor.reset(new PrivateDataIndicatorDescriptor());
      }
      else if (descriptor_tag == 0x10)
      {
        descriptor.reset(new SmoothingBufferDescriptor());
      }
      else if (descriptor_tag == 0x11)
      {
        descriptor.reset(new STDDescriptor());
      }
      else if (descriptor_tag == 0x12)
      {
        descriptor.reset(new IBPDescriptor());
      }
      // 0x13 - 0x1A defined in ISO/IEC 13818-6
      else if (descriptor_tag == 0x1B)
      {
        descriptor.reset(new MPEG4VideoDescriptor());
      }
      else if (descriptor_tag == 0x1C)
      {
        descriptor.reset(new MPEG4AudioDescriptor());
      }
      else if (descriptor_tag == 0x1D)
      {
        descriptor.reset(new IODDescriptor());
      }
      else if (descriptor_tag == 0x1E)
      {
        descriptor.reset(new SLDescriptor());
      }
      else if (descriptor_tag == 0x1F)
      {
        descriptor.reset(new FMCDescriptor());
      }
      else if (descriptor_tag == 0x20)
      {
        descriptor.reset(new ExternalESIDDescriptor());
      }
      else if (descriptor_tag == 0x21)
      {
        descriptor.reset(new MuxcodeDescriptor());
      }
      else if (descriptor_tag == 0x23)
      {
        descriptor.reset(new MultiplexBufferDescriptor());
      }
      else if (descriptor_tag == 0x2C)
      {
        descriptor.reset(new FlexMuxTimingDescriptor());
      }
      else if (descriptor_tag == 0x34)
      {
        descriptor.reset(new MPEG2StereoscopicVideoFormatDescriptor());
      }
      else if (descriptor_tag == 0x80)
      {
        // stuffing descriptor:
        descriptor.reset(new Descriptor());
      }
      else if (descriptor_tag == 0x81)
      {
        descriptor.reset(new AC3AudioDescriptor());
      }
      else if (descriptor_tag == 0x86)
      {
        descriptor.reset(new CaptionServiceDescriptor());
      }
      else if (descriptor_tag == 0x87)
      {
        descriptor.reset(new ContentAdvisoryDescriptor());
      }
      else if (descriptor_tag == 0xA0)
      {
        descriptor.reset(new ExtendedChannelNameDescriptor());
      }
      else if (descriptor_tag == 0xA1)
      {
        descriptor.reset(new ServiceLocationDescriptor());
      }
      else if (descriptor_tag == 0xA2)
      {
        descriptor.reset(new TimeShiftedServiceDescriptor());
      }
      else if (descriptor_tag == 0xA3)
      {
        descriptor.reset(new ComponentNameDescriptor());
      }
      else if (descriptor_tag == 0xA8)
      {
        // DCC departing request descriptor:
        descriptor.reset(new DCCRequestDescriptor());
      }
      else if (descriptor_tag == 0xA9)
      {
        // DCC arriving request descriptor:
        descriptor.reset(new DCCRequestDescriptor());
      }
      else if (descriptor_tag == 0xAA)
      {
        descriptor.reset(new RedistributionControlDescriptor());
      }
      else if (descriptor_tag == 0xAB)
      {
        descriptor.reset(new GenreDescriptor());
      }
      else if (descriptor_tag == 0xAD)
      {
        // ATSC private information descriptor:
        descriptor.reset(new Descriptor());
      }
      else if (descriptor_tag == 0xCC)
      {
        descriptor.reset(new EAC3AudioStreamDescriptor());
      }
      else if (descriptor_tag >= 0x37 &&
               descriptor_tag <= 0x3F)
      {
        // Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved
        descriptor.reset(new RawDescriptor());
      }
      else if (descriptor_tag >= 0x40)
      {
        // user private:
        descriptor.reset(new RawDescriptor());
      }
      else
      {
        yae_elog("FIXME: unimplemented descriptor: 0x%s",
                 yae::to_hex(&descriptor_tag, 1).c_str());
        descriptor.reset(new RawDescriptor());
      }

      YAE_THROW_IF(!descriptor);
      descriptor->load(bin);
      return descriptor;
    }


    //----------------------------------------------------------------
    // Section::Section
    //
    Section::Section():
      pointer_field_(0),
      table_id_(0),
      section_syntax_indicator_(0),
      private_indicator_(0),
      reserved1_(0),
      section_length_(0),
      transport_stream_id_(0),
      reserved2_(0),
      version_number_(0),
      current_next_indicator_(0),
      section_number_(0),
      last_section_number_(0),
      crc32_(0)
    {}

    //----------------------------------------------------------------
    // Section::load_header
    //
    void
    Section::load_header(IBitstream & bin)
    {
      pointer_field_ = bin.read(8);
      bin.skip_bytes(pointer_field_);

      table_id_ = bin.read(8);

      section_syntax_indicator_ = bin.read(1);
      YAE_THROW_IF(section_syntax_indicator_ != 0x1);

      private_indicator_ = bin.read(1);
      reserved1_ = bin.read(2);
      YAE_THROW_IF(reserved1_ != 0x3);

      section_length_ = bin.read(12);
      YAE_THROW_IF(!bin.has_enough_bytes(section_length_));

      transport_stream_id_ = bin.read(16);
      reserved2_ = bin.read(2);
      YAE_THROW_IF(reserved2_ != 0x3);

      version_number_ = bin.read(5);
      current_next_indicator_ = bin.read(1);
      section_number_ = bin.read(8);
      last_section_number_ = bin.read(8);
    }

    //----------------------------------------------------------------
    // Section::load
    //
    void
    Section::load(IBitstream & bin)
    {
      this->load_header(bin);

      std::size_t n_bytes = section_length_ - 9;
      this->load_body(bin, n_bytes);

      crc32_ = bin.read(32);
    }


    //----------------------------------------------------------------
    // PrivateSection::load
    //
    void
    PrivateSection::load(IBitstream & bin)
    {
      pointer_field_ = bin.read(8);
      bin.skip_bytes(pointer_field_);

      table_id_ = bin.read(8);
      section_syntax_indicator_ = bin.read(1);
      private_indicator_ = bin.read(1);
      reserved1_ = bin.read(2);
      YAE_THROW_IF(reserved1_ != 0x3);

      section_length_ = bin.read(12);
      YAE_THROW_IF(!bin.has_enough_bytes(section_length_));

      if (section_syntax_indicator_)
      {
        table_id_extension_ = bin.read(16);
        reserved2_ = bin.read(2);
        YAE_THROW_IF(reserved2_ != 0x3);

        version_number_ = bin.read(5);
        current_next_indicator_ = bin.read(1);
        section_number_ = bin.read(8);
        last_section_number_ = bin.read(8);

        private_data_ = bin.read_bytes(section_length_ - 9);

        crc32_ = bin.read(32);
      }
      else
      {
        private_data_ = bin.read_bytes(section_length_);
      }
    }


    //----------------------------------------------------------------
    // TSDescriptionSection::load_body
    //
    void
    TSDescriptionSection::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      std::size_t body_end = bin.position_plus_nbytes(n_bytes);

      descriptor_.clear();
      while (bin.position() < body_end)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != body_end);
    }


    //----------------------------------------------------------------
    // ProgramAssociationTable::load_body
    //
    void
    ProgramAssociationTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      YAE_THROW_IF((n_bytes & 0x3) != 0);

      std::size_t n = n_bytes >> 2;
      program_.resize(n);
      for (std::size_t i = 0; i < n; i++)
      {
        Program & p = program_[i];
        p.load(bin);
      }
    }


    //----------------------------------------------------------------
    // ProgramAssociationTable::Program::Program
    //
    ProgramAssociationTable::Program::Program():
      program_number_(0),
      reserved_(0),
      pid_(0)
    {}

    //----------------------------------------------------------------
    // ProgramAssociationTable::Program::load
    //
    void
    ProgramAssociationTable::Program::load(IBitstream & bin)
    {
      program_number_ = bin.read(16);
      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);
      pid_ = bin.read(13);
    }


    //----------------------------------------------------------------
    // ConditionalAccessTable::load_body
    //
    void
    ConditionalAccessTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      std::size_t body_end = bin.position_plus_nbytes(n_bytes);

      descriptor_.clear();
      while (bin.position() < body_end)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != body_end);
    }


    //----------------------------------------------------------------
    // ProgramMapTable::ProgramMapTable
    //
    ProgramMapTable::ProgramMapTable():
      reserved1_(0),
      pcr_pid_(0),
      reserved2_(0),
      program_info_length_(0)
    {}

    //----------------------------------------------------------------
    // ProgramMapTable::load_body
    //
    void
    ProgramMapTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      std::size_t body_end = bin.position_plus_nbytes(n_bytes);

      reserved1_ = bin.read(3);
      YAE_THROW_IF(reserved1_ != 0x7);

      pcr_pid_ = bin.read(13);
      reserved2_ = bin.read(4);
      YAE_THROW_IF(reserved2_ != 0xF);

      program_info_length_ = bin.read(12);
      descriptor_.clear();

      std::size_t stop_pos = bin.position_plus_nbytes(program_info_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);

      while (bin.position() < body_end)
      {
        es_.push_back(ElementaryStream());
        ElementaryStream & es = es_.back();
        es.load(bin);
      }
      YAE_THROW_IF(bin.position() != body_end);
    }

    //----------------------------------------------------------------
    // ProgramMapTable::ElementaryStream::ElementaryStream
    //
    ProgramMapTable::ElementaryStream::ElementaryStream():
      stream_type_(0),
      reserved1_(0),
      elementary_pid_(0),
      reserved2_(0),
      es_info_length_(0)
    {}

    //----------------------------------------------------------------
    // ProgramMapTable::ElementaryStream::load
    //
    void
    ProgramMapTable::ElementaryStream::load(IBitstream & bin)
    {
      stream_type_ = bin.read(8);
      reserved1_ = bin.read(3);
      YAE_THROW_IF(reserved1_ != 0x7);

      elementary_pid_ = bin.read(13);
      reserved2_ = bin.read(4);
      YAE_THROW_IF(reserved2_ != 0xF);

      es_info_length_ = bin.read(12);
      descriptor_.clear();

      std::size_t stop_pos = bin.position_plus_nbytes(es_info_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }


    //----------------------------------------------------------------
    // SystemTimeTable::SystemTimeTable
    //
    SystemTimeTable::SystemTimeTable():
      protocol_version_(0),
      system_time_(0),
      gps_utc_offset_(0),
      daylight_saving_status_(0),
      daylight_saving_reserved_(0),
      daylight_saving_day_of_month_(0),
      daylight_saving_hour_(0)
    {}

    //----------------------------------------------------------------
    // SystemTimeTable::load_body
    //
    void
    SystemTimeTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      std::size_t stop_pos = bin.position_plus_nbytes(n_bytes);

      protocol_version_ = bin.read(8);
      system_time_ = bin.read(32);
      gps_utc_offset_ = bin.read(8);

      daylight_saving_status_ = bin.read(1);
      daylight_saving_reserved_ = bin.read(2);
      YAE_THROW_IF(daylight_saving_reserved_ != 0x3);

      daylight_saving_day_of_month_ = bin.read(5);
      daylight_saving_hour_ = bin.read(8);

      descriptor_.clear();
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);

      // FIXME: pkoshevoy:
      static const int64_t unix_epoch_gps_offset = // 315964800;
        yae::unix_epoch_time_at_utc_time(1980, 1, 6, 0, 0, 0);
      time_t t_epoch = unix_epoch_gps_offset + system_time_ - gps_utc_offset_;

      struct tm t = { 0 };
      localtime_r(&t_epoch, &t);

      std::cerr
        << "\nSystemTimeTable, utc time: "
        << yae::strfmt("%04i/%02i/%02i %02i:%02i:%02i\n",
                       t.tm_year + 1900,
                       t.tm_mon + 1,
                       t.tm_mday,
                       t.tm_hour,
                       t.tm_min,
                       t.tm_sec)
        << std::endl;
    }


    //----------------------------------------------------------------
    // MasterGuideTable::MasterGuideTable
    //
    MasterGuideTable::MasterGuideTable():
      protocol_version_(0)
    {}

    //----------------------------------------------------------------
    // MasterGuideTable::load_body
    //
    void
    MasterGuideTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      protocol_version_ = bin.read(8);
      tables_defined_ = bin.read(16);

      table_.resize(tables_defined_);
      for (std::size_t i = 0; i < tables_defined_; i++)
      {
        Table & table = table_[i];
        table.load(bin);
      }

      reserved_ = bin.read(4);
      // YAE_THROW_IF(reserved_ != 0xF);

      descriptors_length_ = bin.read(12);
      descriptor_.clear();

      std::size_t stop_pos = bin.position_plus_nbytes(descriptors_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }

    //----------------------------------------------------------------
    // MasterGuideTable::Table::Table
    //
    MasterGuideTable::Table::Table():
      table_type_(0),
      reserved1_(0),
      table_type_pid_(0),
      reserved2_(0),
      table_type_version_number_(0),
      number_bytes_(0),
      reserved3_(0),
      table_type_descriptors_length_(0)
    {}

    //----------------------------------------------------------------
    // MasterGuideTable::Table::load
    //
    void
    MasterGuideTable::Table::load(IBitstream & bin)
    {
      table_type_ = bin.read(16);
      reserved1_ = bin.read(3);
      YAE_THROW_IF(reserved1_ != 0x7);

      table_type_pid_ = bin.read(13);
      reserved2_ = bin.read(3);
      YAE_THROW_IF(reserved2_ != 0x7);

      table_type_version_number_ = bin.read(5);
      number_bytes_ = bin.read(32);
      reserved3_ = bin.read(4);
      YAE_THROW_IF(reserved3_ != 0xF);

      table_type_descriptors_length_ = bin.read(12);
      descriptor_.clear();

      std::size_t stop_pos =
        bin.position_plus_nbytes(table_type_descriptors_length_);

      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }

      YAE_THROW_IF(bin.position() != stop_pos);
    }



    //----------------------------------------------------------------
    // VirtualChannelTable::VirtualChannelTable
    //
    VirtualChannelTable::VirtualChannelTable():
      protocol_version_(0),
      num_channels_in_section_(0),
      reserved_(0),
      additional_descriptors_length_(0)
    {}

    //----------------------------------------------------------------
    // VirtualChannelTable::load_body
    //
    void
    VirtualChannelTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      protocol_version_ = bin.read(8);
      num_channels_in_section_ = bin.read(8);

      channel_.resize(num_channels_in_section_);
      for (std::size_t i = 0; i < num_channels_in_section_; i++)
      {
        Channel & channel = channel_[i];
        channel.load(bin);

        if (table_id_ == 0xC8)
        {
          int reserved =
            (channel.path_selected_ << 1) |
            (channel.out_of_band_);

          YAE_THROW_IF(reserved != 0x3);
        }
      }

      reserved_ = bin.read(6);
      // YAE_THROW_IF(reserved_ != 0x3F);

      additional_descriptors_length_ = bin.read(10);
      additional_descriptor_.clear();

      std::size_t stop_pos =
        bin.position_plus_nbytes(additional_descriptors_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        additional_descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }

    //----------------------------------------------------------------
    // VirtualChannelTable::Channel::Channel
    //
    VirtualChannelTable::Channel::Channel():
      reserved1_(0),
      major_channel_number_(0),
      minor_channel_number_(0),
      modulation_mode_(0),
      carrier_frequency_(0),
      channel_tsid_(0),
      program_number_(0),
      etm_location_(0),
      access_controlled_(0),
      hidden_(0),
      path_selected_(0),
      out_of_band_(0),
      hide_guide_(0),
      reserved3_(0),
      service_type_(0),
      source_id_(0),
      reserved4_(0),
      descriptors_length_(0)
    {
      memset(short_name_, 0, sizeof(short_name_));
    }

    //----------------------------------------------------------------
    // VirtualChannelTable::Channel::load
    //
    void
    VirtualChannelTable::Channel::load(IBitstream & bin)
    {
      bin.read<uint16_t>(short_name_, 7);
      reserved1_ = bin.read(4);
      YAE_THROW_IF(reserved1_ != 0xF);

      major_channel_number_ = bin.read(10);
      YAE_THROW_IF(major_channel_number_ < 1 || major_channel_number_ > 99);

      minor_channel_number_ = bin.read(10);
      YAE_THROW_IF(minor_channel_number_ > 999);

      modulation_mode_ = bin.read(8);
      carrier_frequency_ = bin.read(32);
      channel_tsid_ = bin.read(16);
      program_number_ = bin.read(16);
      etm_location_ = bin.read(2);
      access_controlled_ = bin.read(1);
      hidden_ = bin.read(1);

      // this is a 2-bit reserved field in TVCT with expected value '11',
      // and 2 separate 1-bit fields in CVCT:
      path_selected_ = bin.read(1);
      out_of_band_ = bin.read(1);

      hide_guide_ = bin.read(1);
      reserved3_ = bin.read(3);
      YAE_THROW_IF(reserved3_ != 0x7);

      service_type_ = bin.read(6);
      source_id_ = bin.read(16);
      reserved4_ = bin.read(6);
      YAE_THROW_IF(reserved4_ != 0x3F);

      descriptors_length_ = bin.read(10);
      descriptor_.clear();

      std::size_t stop_pos = bin.position_plus_nbytes(descriptors_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }


    //----------------------------------------------------------------
    // RatingRegionTable::RatingRegionTable
    //
    RatingRegionTable::RatingRegionTable():
      protocol_version_(0),
      rating_region_name_length_(0),
      dimensions_defined_(0),
      reserved_(0),
      descriptors_length_(0)
    {}

    //----------------------------------------------------------------
    // RatingRegionTable::load_body
    //
    void
    RatingRegionTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      protocol_version_ = bin.read(8);
      rating_region_name_length_ = bin.read(8);

      std::size_t stop_pos =
        bin.position_plus_nbytes(rating_region_name_length_);
      rating_region_name_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "RatingRegionTable, rating region name: "
        << rating_region_name_text_.to_str()
        << std::endl;

      dimensions_defined_ = bin.read(8);
      dimension_.resize(dimensions_defined_);
      for (std::size_t i = 0; i < dimensions_defined_; ++i)
      {
        Dimension & dimension = dimension_[i];
        dimension.load(bin);
      }

      reserved_ = bin.read(6);
      YAE_THROW_IF(reserved_ != 0x3F);

      descriptors_length_ = bin.read(10);
      descriptor_.clear();

      stop_pos = bin.position_plus_nbytes(descriptors_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }

    //----------------------------------------------------------------
    // RatingRegionTable::Dimension::Dimension
    //
    RatingRegionTable::Dimension::Dimension():
      dimension_name_length_(0),
      reserved_(0),
      graduated_scale_(0),
      values_defined_(0)
    {}

    //----------------------------------------------------------------
    // RatingRegionTable::Dimension::load
    //
    void
    RatingRegionTable::Dimension::load(IBitstream & bin)
    {
      dimension_name_length_ = bin.read(8);
      std::size_t stop_pos = bin.position_plus_nbytes(dimension_name_length_);
      dimension_name_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "RatingRegionTable::Dimension, dimension name: "
        << dimension_name_text_.to_str()
        << std::endl;

      reserved_ = bin.read(3);
      YAE_THROW_IF(reserved_ != 0x7);

      graduated_scale_ = bin.read(1);
      values_defined_ = bin.read(4);

      rating_.resize(values_defined_);
      for (std::size_t i = 0; i < values_defined_; i++)
      {
        Rating & rating = rating_[i];
        rating.load(bin);
      }
    }

    //----------------------------------------------------------------
    // RatingRegionTable::Dimension::Rating::Rating
    //
    RatingRegionTable::Dimension::Rating::Rating():
      abbrev_rating_value_length_(0),
      rating_value_length_(0)
    {}

    //----------------------------------------------------------------
    // RatingRegionTable::Dimension::Rating::load
    //
    void
    RatingRegionTable::Dimension::Rating::load(IBitstream & bin)
    {
      abbrev_rating_value_length_ = bin.read(8);
      std::size_t stop_pos =
        bin.position_plus_nbytes(abbrev_rating_value_length_);
      abbrev_rating_value_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "RatingRegionTable::Dimension::Rating, abbrev value: "
        << abbrev_rating_value_text_.to_str()
        << std::endl;

      rating_value_length_ = bin.read(8);
      stop_pos = bin.position_plus_nbytes(rating_value_length_);
      rating_value_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "RatingRegionTable::Dimension::Rating, rating value: "
        << rating_value_text_.to_str()
        << std::endl;
    }


    //----------------------------------------------------------------
    // EventInformationTable::EventInformationTable
    //
    EventInformationTable::EventInformationTable():
      protocol_version_(0),
      num_events_in_section_(0)
    {}

    //----------------------------------------------------------------
    // EventInformationTable::load_body
    //
    void
    EventInformationTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      protocol_version_ = bin.read(8);
      num_events_in_section_ = bin.read(8);

      event_.resize(num_events_in_section_);
      for (std::size_t i = 0; i < num_events_in_section_; i++)
      {
        Event & event = event_[i];
        event.load(bin);
      }
    }

    //----------------------------------------------------------------
    // EventInformationTable::Event::Event
    //
    EventInformationTable::Event::Event():
      reserved1_(0),
      event_id_(0),
      start_time_(0),
      reserved2_(0),
      etm_location_(0),
      length_in_seconds_(0),
      title_length_(0),
      reserved3_(0),
      descriptors_length_(0)
    {}

    //----------------------------------------------------------------
    // EventInformationTable::Event::load
    //
    void
    EventInformationTable::Event::load(IBitstream & bin)
    {
      reserved1_ = bin.read(2);
      YAE_THROW_IF(reserved1_ != 0x3);

      event_id_ = bin.read(14);
      start_time_ = bin.read(32);
      reserved2_ = bin.read(2);
      YAE_THROW_IF(reserved2_ != 0x3);

      etm_location_ = bin.read(2);
      length_in_seconds_ = bin.read(20);
      title_length_ = bin.read(8);
      std::size_t stop_pos = bin.position_plus_nbytes(title_length_);
      title_text_.load(bin);
      YAE_THROW_IF(bin.position() > stop_pos);
      YAE_ASSERT(bin.position() == stop_pos);
      bin.seek(stop_pos);

      // FIXME: pkoshevoy:
      std::cerr
        << "EventInformationTable::Event, title text: "
        << title_text_.to_str()
        << std::endl;

      reserved3_ = bin.read(4);
      YAE_THROW_IF(reserved3_ != 0xF);

      descriptors_length_ = bin.read(12);
      descriptor_.clear();

      stop_pos = bin.position_plus_nbytes(descriptors_length_);
      while (bin.position() < stop_pos)
      {
        TDescriptorPtr descriptor = load_descriptor(bin);
        descriptor_.push_back(descriptor);
      }
      YAE_THROW_IF(bin.position() != stop_pos);
    }


    //----------------------------------------------------------------
    // ExtendedTextTable::ExtendedTextTable
    //
    ExtendedTextTable::ExtendedTextTable():
      protocol_version_(0),
      etm_id_(0)
    {}

    //----------------------------------------------------------------
    // ExtendedTextTable::load_body
    //
    void
    ExtendedTextTable::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      protocol_version_ = bin.read(8);
      etm_id_ = bin.read(32);
      extended_text_message_.load(bin);

      // FIXME: pkoshevoy:
      std::cerr
        << "ExtendedTextTable, extended text message: "
        << extended_text_message_.to_str()
        << std::endl;
    }


    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceInfoSection
    //
    SpliceInfoSection::SpliceInfoSection():
      protocol_version_(0),
      encrypted_packet_(0),
      encryption_algorithm_(0),
      pts_adjustment_(0),
      cw_index_(0),
      tier_(0),
      splice_command_length_(0),
      splice_command_type_(0),
      descriptor_loop_length_(0),
      ecrc32_(0)
    {}

    //----------------------------------------------------------------
    // decrypt
    //
    static TBufferPtr
    decrypt(const TBufferPtr & payload, uint64_t encryption_algorithm)
    {
      throw std::runtime_error("decryption not implemented");
      return payload;
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::load
    //
    void
    SpliceInfoSection::load(IBitstream & bin)
    {
      pointer_field_ = bin.read(8);
      bin.skip_bytes(pointer_field_);

      table_id_ = bin.read(8);

      section_syntax_indicator_ = bin.read(1);
      YAE_THROW_IF(section_syntax_indicator_ != 0x0);

      private_indicator_ = bin.read(1);
      YAE_THROW_IF(section_syntax_indicator_ != 0x0);

      reserved1_ = bin.read(2);
      YAE_THROW_IF(reserved1_ != 0x3);

      section_length_ = bin.read(12);
      YAE_THROW_IF(!bin.has_enough_bytes(section_length_));
      std::size_t start_pos = bin.position();

      protocol_version_ = bin.read(8);
      encrypted_packet_ = bin.read(1);
      encryption_algorithm_ = bin.read(6);
      pts_adjustment_ = bin.read(33);
      cw_index_ = bin.read(8);
      tier_ = bin.read(12);
      splice_command_length_ = bin.read(12);

      std::size_t consumed_bytes = (bin.position() - start_pos) >> 3;
      std::size_t payload_size = section_length_ - consumed_bytes - 4;
      TBufferPtr payload = bin.read_bytes(payload_size);
      if (encrypted_packet_)
      {
        payload = decrypt(payload, encryption_algorithm_);
      }

      yae::Bitstream decrypted(payload);
      splice_command_type_ = decrypted.read(8);

      if (splice_command_type_ == 0x00)
      {
        command_.reset(new SpliceNull());
      }
      else if (splice_command_type_ == 0x04)
      {
        command_.reset(new SpliceSchedule());
      }
      else if (splice_command_type_ == 0x05)
      {
        command_.reset(new SpliceInsert());
      }
      else if (splice_command_type_ == 0x06)
      {
        command_.reset(new TimeSignal());
      }
      else if (splice_command_type_ == 0x07)
      {
        command_.reset(new BandwidthReservation());
      }
      else if (splice_command_type_ == 0xFF)
      {
        command_.reset(new PrivateCommand());
      }

      std::size_t cmd_bytes =
        (splice_command_length_ == 0xFFF) ?
        (payload_size - (encrypted_packet_ ? 5 : 1)) :
        splice_command_length_;

      std::size_t cmd_end = decrypted.position_plus_nbytes(cmd_bytes);
      if (command_)
      {
        command_->load(decrypted, cmd_bytes);
      }

      YAE_THROW_IF(cmd_end < decrypted.position());

      if (splice_command_length_ != 0xFFF)
      {
        decrypted.seek(cmd_end);
      }

      descriptor_loop_length_ = decrypted.read(16);
      descriptor_.resize(descriptor_loop_length_);

      for (std::size_t i = 0, n = descriptor_.size(); i < n; i++)
      {
        SpliceDescriptor & descriptor = descriptor_[i];
        descriptor.load(decrypted);
      }

      consumed_bytes = decrypted.position() >> 3;
      std::size_t stuffing_bytes = payload_size - consumed_bytes;
      if (stuffing_bytes)
      {
        if (encrypted_packet_)
        {
          // ecrc32:
          YAE_THROW_IF(stuffing_bytes < 4);
          stuffing_bytes -= 4;
        }

        alignment_stuffing_ = decrypted.read_bytes(stuffing_bytes);

        if (encrypted_packet_)
        {
          ecrc32_ = decrypted.read(32);
        }
      }

      crc32_ = bin.read(32);
    }


    //----------------------------------------------------------------
    // SpliceInfoSection::BreakDuration::BreakDuration
    //
    SpliceInfoSection::BreakDuration::BreakDuration():
      auto_return_(0),
      reserved_(0),
      duration_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::BreakDuration::load
    //
    void
    SpliceInfoSection::BreakDuration::load(IBitstream & bin)
    {
      auto_return_ = bin.read(1);
      reserved_ = bin.read(6);
      duration_ = bin.read(33);
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceTime::SpliceTime
    //
    SpliceInfoSection::SpliceTime::SpliceTime():
      time_specified_flag_(0),
      reserved_(0),
      pts_time_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceTime::load
    //
    void
    SpliceInfoSection::SpliceTime::load(IBitstream & bin)
    {
      time_specified_flag_ = bin.read(1);
      reserved_ = bin.read(6);
      if (time_specified_flag_)
      {
        pts_time_ = bin.read(33);
      }
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::Splice::Splice
    //
    SpliceInfoSection::Splice::Splice():
      splice_event_id_(0),
      splice_event_cancel_indicator_(0),
      reserved1_(0),
      out_of_network_indicator_(0),
      program_splice_flag_(0),
      duration_flag_(0),
      reserved2_(0),
      utc_splice_time_(0),
      component_count_(0),
      unique_program_id_(0),
      avail_num_(0),
      avails_expected_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::Splice::load
    //
    void
    SpliceInfoSection::Splice::load(IBitstream & bin)
    {
      splice_event_id_ = bin.read(32);
      splice_event_cancel_indicator_ = bin.read(1);
      reserved1_ = bin.read(7);

      if (!splice_event_cancel_indicator_)
      {
        out_of_network_indicator_ = bin.read(1);
        program_splice_flag_ = bin.read(1);
        duration_flag_ = bin.read(1);
        reserved2_ = bin.read(5);

        if (program_splice_flag_)
        {
          utc_splice_time_ = bin.read(32);
        }
        else
        {
          component_count_ = bin.read(8);
          component_.resize(component_count_);
          for (std::size_t i = 0, n = component_.size(); i < n; i++)
          {
            Component & component = component_[i];
            component.component_tag_ = bin.read(8);
            component.utc_splice_time_ = bin.read(32);
          }
        }

        if (duration_flag_)
        {
          break_duration_ = BreakDuration();
          break_duration_->load(bin);
        }

        unique_program_id_ = bin.read(16);
        avail_num_ = bin.read(8);
        avails_expected_ = bin.read(8);
      }
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceSchedule::SpliceSchedule
    //
    SpliceInfoSection::SpliceSchedule::SpliceSchedule():
      splice_count_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceSchedule::load
    //
    void
    SpliceInfoSection::SpliceSchedule::load(IBitstream & bin,
                                            std::size_t nbytes)
    {
      splice_count_ = bin.read(8);
      splice_.resize(splice_count_);
      for (std::size_t i = 0, n = splice_.size(); i < n; i++)
      {
        Splice & splice = splice_[i];
        splice.load(bin);
      }

    }

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceInsert::SpliceInsert
    //
    SpliceInfoSection::SpliceInsert::SpliceInsert():
      splice_event_id_(0),
      splice_event_cancel_indicator_(0),
      reserved1_(0),
      out_of_network_indicator_(0),
      program_splice_flag_(0),
      duration_flag_(0),
      splice_immediate_flag_(0),
      reserved2_(0),
      component_count_(0),
      unique_program_id_(0),
      avail_num_(0),
      avails_expected_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceInsert::load
    //
    void
    SpliceInfoSection::SpliceInsert::load(IBitstream & bin,
                                          std::size_t nbytes)
    {
      splice_event_id_ = bin.read(32);
      splice_event_cancel_indicator_ = bin.read(1);
      reserved1_ = bin.read(7);

      if (!splice_event_cancel_indicator_)
      {
        out_of_network_indicator_ = bin.read(1);
        program_splice_flag_ = bin.read(1);
        duration_flag_ = bin.read(1);
        splice_immediate_flag_ = bin.read(1);
        reserved2_ = bin.read(4);

        if (program_splice_flag_ && !splice_immediate_flag_)
        {
          splice_time_ = SpliceTime();
          splice_time_->load(bin);
        }

        if (!program_splice_flag_)
        {
          component_count_ = bin.read(8);
          component_.resize(component_count_);

          for (std::size_t i = 0, n = component_.size(); i < n; i++)
          {
            Component & component = component_[i];
            component.component_tag_ = bin.read(8);
            if (!splice_immediate_flag_)
            {
              component.splice_time_ = SpliceTime();
              component.splice_time_->load(bin);
            }
          }
        }

        if (duration_flag_)
        {
          break_duration_ = BreakDuration();
          break_duration_->load(bin);
        }

        unique_program_id_ = bin.read(16);
        avail_num_ = bin.read(8);
        avails_expected_ = bin.read(8);
      }
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::TimeSignal::load
    //
    void
    SpliceInfoSection::TimeSignal::load(IBitstream & bin,
                                        std::size_t nbytes)
    {
      splice_time_.load(bin);
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::PrivateCommand::PrivateCommand
    //
    SpliceInfoSection::PrivateCommand::PrivateCommand():
      identifier_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::PrivateCommand::load
    //
    void
    SpliceInfoSection::PrivateCommand::load(IBitstream & bin,
                                            std::size_t nbytes)
    {
      identifier_ = bin.read(32);
      private_ = bin.read_bytes(nbytes - 4);
    }

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceDescriptor::SpliceDescriptor
    //
    SpliceInfoSection::SpliceDescriptor::SpliceDescriptor():
      splice_descriptor_tag_(0),
      descriptor_length_(0),
      identified_(0)
    {}

    //----------------------------------------------------------------
    // SpliceInfoSection::SpliceDescriptor::load
    //
    void
    SpliceInfoSection::SpliceDescriptor::load(IBitstream & bin)
    {
      splice_descriptor_tag_ = bin.read(8);
      descriptor_length_ = bin.read(8);
      identified_ = bin.read(32);
      private_ = bin.read_bytes(descriptor_length_ - 4);
    }

    //----------------------------------------------------------------
    // DSMCCSection::load_body
    //
    void
    DSMCCSection::load_body(IBitstream & bin, std::size_t n_bytes)
    {
      body_ = bin.read_bytes(n_bytes);
    }


    //----------------------------------------------------------------
    // load_section
    //
    TSectionPtr
    load_section(IBitstream & bin)
    {
      std::size_t start_pos = bin.position();
      uint8_t pointer_field = bin.read(8);
      bin.skip_bytes(pointer_field);
      uint8_t table_id = bin.read(8);
      bin.seek(start_pos);

      TSectionPtr section;
      if (table_id == 0x00)
      {
        // PAT
        section.reset(new ProgramAssociationTable());
      }
      else if (table_id == 0x01)
      {
        // CAT
        section.reset(new ConditionalAccessTable());
      }
      else if (table_id == 0x02)
      {
        // PMT
        section.reset(new ProgramMapTable());
      }
      else if (table_id == 0xC7)
      {
        // MGT
        section.reset(new MasterGuideTable());
      }
      else if (table_id == 0xC8)
      {
        // TVCT
        section.reset(new VirtualChannelTable());
      }
      else if (table_id == 0xC9)
      {
        // CVCT
        section.reset(new VirtualChannelTable());
      }
      else if (table_id == 0xCA)
      {
        // RRT
        section.reset(new RatingRegionTable());
      }
      else if (table_id == 0xCB)
      {
        // EIT
        section.reset(new EventInformationTable());
      }
      else if (table_id == 0xCC)
      {
        // ETT
        section.reset(new ExtendedTextTable());
      }
      else if (table_id == 0xCD)
      {
        // STT
        section.reset(new SystemTimeTable());
      }
      else if (table_id == 0xFC)
      {
        // SCTE-35
        section.reset(new SpliceInfoSection());
      }
      else if (table_id >= 0x3A && table_id <= 0x3F)
      {
        // DSM-CC:
        // 0x3A ISO/IEC 13818-6 DSM CC multiprotocol encapsulated.
        // 0x3B ISO/IEC 13818-6 DSM CC U-N messages.
        // 0x3C ISO/IEC 13818-6 DSM CC Download Data Messages.
        // 0x3D ISO/IEC 13818-6 DSM CC stream descriptor list.
        // 0x3E ISO/IEC 13818-6 DSM CC privately defined
        //      (DVB MAC addressed datagram).
        // 0x3F	ISO/IEC 13818-6 DSM CC addressable.
        section.reset(new DSMCCSection());
      }
      else if (table_id >= 0x40 && table_id <= 0xFE)
      {
        // User Private:
        section.reset(new PrivateSection());
      }

      YAE_THROW_IF(!section);
      section->load(bin);
      return section;
    }


    //----------------------------------------------------------------
    // assemble_payload
    //
    yae::Data
    assemble_payload(std::list<TSPacket> & packets)
    {
      std::size_t payload_bytes = 0;
      for (std::list<TSPacket>::const_iterator i = packets.begin();
           i != packets.end(); ++i)
      {
        const TSPacket & pkt = *i;
        if (pkt.payload_)
        {
          payload_bytes += pkt.payload_->size();
        }
      }

      Data payload(payload_bytes);
      unsigned char * dst = payload.get();

      for (std::list<TSPacket>::const_iterator i = packets.begin();
           i != packets.end(); ++i)
      {
        const TSPacket & pkt = *i;
        if (pkt.payload_)
        {
          const unsigned char * src = pkt.payload_->get();
          const std::size_t src_bytes = pkt.payload_->size();
          memcpy(dst, src, src_bytes);
          dst += src_bytes;
        }
      }

      return payload;
    }

    //----------------------------------------------------------------
    // Context::consume
    //
    void
    Context::consume(uint16_t pid, std::list<TSPacket> & packets, bool parse)
    {
      yae::Data payload = assemble_payload(packets);
      std::string tmp = yae::to_hex(payload.get(),
                                    std::min<std::size_t>(payload.size(), 32),
                                    4);
      yae_dlog("%5i (0x%04X) pid   %8i pkts   %8i bytes   %s ...",
               int(pid),
               int(pid),
               int(packets.size()),
               int(payload.size()),
               tmp.c_str());

      if (!parse)
      {
        return;
      }

      try
      {
        yae::Bitstream bin(payload);
        if (pid == 0x0000)
        {
          PATSectionPtr section = load_section(bin);
          const ProgramAssociationTable & pat = *section;
          YAE_THROW_IF(pat.table_id_ != 0x00);
          YAE_THROW_IF(pat.private_indicator_ != 0);

          for (std::size_t i = 0, n = pat.program_.size(); i < n; i++)
          {
            const ProgramAssociationTable::Program & p = pat.program_[i];
            if (p.program_number_)
            {
              pid_pmt_[p.pid_] = p.program_number_;
            }
            else
            {
              network_pid_ = p.pid_;
            }
          }
        }
        else if (pid == 0x0001)
        {
          CATSectionPtr section = load_section(bin);
          const ConditionalAccessTable & cat = *section;
          YAE_THROW_IF(cat.table_id_ != 0x01);
          YAE_THROW_IF(cat.private_indicator_ != 0);
        }
        else if (pid == 0x0002)
        {
          TSDescSectionPtr section = load_section(bin);
          const TSDescriptionSection & tsd = *section;
          YAE_THROW_IF(tsd.table_id_ != 0x02);
          YAE_THROW_IF(tsd.private_indicator_ != 0);
        }
        else if (yae::has(pid_pmt_, pid))
        {
          TSectionPtr section = load_section(bin);
          PMTSectionPtr pmt_section = section;
          YAE_EXPECT(pmt_section);
          if (pmt_section)
          {
            const ProgramMapTable & pmt = *pmt_section;
            YAE_THROW_IF(pmt.table_id_ != 0x02);
            YAE_THROW_IF(pmt.private_indicator_ != 0);

            for (std::size_t i = 0, n = pmt.es_.size(); i < n; i++)
            {
              const ProgramMapTable::ElementaryStream & es = pmt.es_[i];
              pid_es_[es.elementary_pid_] = pmt.program_number_;
            }
          }
        }
        else if (pid == 0x1FFB)
        {
          TSectionPtr section = load_section(bin);
          MGTSectionPtr mgt_section = section;
          STTSectionPtr stt_section = section;
          VCTSectionPtr vct_section = section;
          RRTSectionPtr rrt_section = section;
          YAE_EXPECT(mgt_section || stt_section || vct_section || rrt_section);

          if (mgt_section)
          {
            const MasterGuideTable & mgt = *mgt_section;
            YAE_THROW_IF(mgt.table_id_ < 0xC7 ||
                         mgt.table_id_ > 0xCD);
            YAE_THROW_IF(mgt.private_indicator_ != 1);

            for (std::size_t i = 0; i < mgt.tables_defined_; i++)
            {
              const MasterGuideTable::Table & table = mgt.table_[i];
              if (table.table_type_ == 0x0000)
              {
                pid_tvct_curr_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ == 0x0001)
              {
                pid_tvct_next_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ == 0x0002)
              {
                pid_cvct_curr_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ == 0x0003)
              {
                pid_cvct_next_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ == 0x0004)
              {
                pid_channel_ett_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ == 0x0005)
              {
                pid_dccsct_.insert(table.table_type_pid_);
              }
              else if (table.table_type_ >= 0x0100 &&
                       table.table_type_ <= 0x017F)
              {
                pid_eit_[table.table_type_pid_] = table.table_type_ & 0x7F;
              }
              else if (table.table_type_ >= 0x0200 &&
                       table.table_type_ <= 0x027F)
              {
                pid_event_ett_[table.table_type_pid_] =
                  table.table_type_ & 0x7F;
              }
              else if (table.table_type_ >= 0x0300 &&
                       table.table_type_ <= 0x03FF)
              {
                pid_rrt_[table.table_type_pid_] = table.table_type_ & 0xFF;
              }
              else if (table.table_type_ >= 0x1400 &&
                       table.table_type_ <= 0x14FF)
              {
                pid_dcct_[table.table_type_pid_] = table.table_type_ & 0xFF;
              }
            }
          }
        }
        else if (yae::has(pid_tvct_curr_, pid))
        {
          VCTSectionPtr section = load_section(bin);
          VirtualChannelTable & vct = *section;
        }
        else if (yae::has(pid_tvct_next_, pid))
        {
          VCTSectionPtr section = load_section(bin);
          VirtualChannelTable & vct = *section;
        }
        else if (yae::has(pid_cvct_curr_, pid))
        {
          VCTSectionPtr section = load_section(bin);
          VirtualChannelTable & vct = *section;
        }
        else if (yae::has(pid_cvct_next_, pid))
        {
          VCTSectionPtr section = load_section(bin);
          VirtualChannelTable & vct = *section;
        }
        else if (yae::has(pid_channel_ett_, pid))
        {
          ETTSectionPtr section = load_section(bin);
          ExtendedTextTable & ett = *section;
        }
        else if (yae::has(pid_dccsct_, pid))
        {
          // FIXME: DCC is not implemented
          TSectionPtr section = load_section(bin);
        }
        else if (yae::has(pid_eit_, pid))
        {
          EITSectionPtr section = load_section(bin);
          EventInformationTable & eit = *section;
        }
        else if (yae::has(pid_event_ett_, pid))
        {
          ETTSectionPtr section = load_section(bin);
          ExtendedTextTable & ett = *section;
        }
        else if (yae::has(pid_rrt_, pid))
        {
          RRTSectionPtr section = load_section(bin);
          RatingRegionTable & rrt = *section;
        }
        else if (yae::has(pid_dcct_, pid))
        {
          // FIXME: DCC is not implemented
          TSectionPtr section = load_section(bin);
        }
        else if (bin.peek_bits(24) == 0x000001)
        {
          PESPacket pes_pkt;
          pes_pkt.load(bin);
        }
        else if (yae::has(pid_es_, pid))
        {
          TSectionPtr section = load_section(bin);
        }
        else
        {
          // not a PES packet, not a PID we recognize -- skip it:
#if 0
          std::string fn = yae::strfmt("/tmp/0x%04X.bin", pid);
          yae::dump(fn, payload.get(), payload.size());
#endif
        }
      }
      catch (const std::exception & e)
      {
        yae_elog("failed to load PESPacket: %s", e.what());
      }
      catch (...)
      {
        yae_elog("failed to load PESPacket: unexpected exception");
      }
    }


    //----------------------------------------------------------------
    // Context::Context
    //
    Context::Context():
      network_pid_(0)
    {}

    //----------------------------------------------------------------
    // Context::load
    //
    void
    Context::load(IBitstream & bin, TSPacket & pkt)
    {
      std::size_t start_pos = bin.position();
      pkt.load(bin, *this);
      std::size_t end_pos = bin.position();
      YAE_THROW_IF(((end_pos - start_pos) >> 3) != 188);

      if (pkt.payload_unit_start_indicator_)
      {
        // The payload_unit_start_indicator has normative meaning for
        // transport stream packets that carry PES packets or transport
        // stream section data.
        //
        // When the payload of the transport stream packet contains
        // PES packet data, the payload_unit_start_indicator has the
        // following significance:
        //
        // - a '1' indicates that the payload of this transport stream packet
        //   will commence with the first byte of a PES packet
        //
        // - a '0' indicates no PES packet shall start in this transport
        //   stream packet.
        //
        // If the payload_unit_start_indicator is set to '1', then one
        // and only one PES packet starts in this transport stream packet.
        // This also applies to private streams of stream_type 6.
        //
        // When the payload of the transport stream packet contains transport
        // stream section data, the payload_unit_start_indicator has the
        // following significance:
        //
        // - if the transport stream packet carries the first byte of a section,
        //   the payload_unit_start_indicator value shall be '1', indicating
        //   that the first byte of the payload of this transport stream
        //   packet carries the pointer_field.
        // - if the transport stream packet does not carry the first byte of
        //   a section, the payload_unit_start_indicator value shall be '0',
        //   indicating that there is no pointer_field in the payload.
        // This also applies to private streams of stream_type 5.
        //
        // For null packets the payload_unit_start_indicator shall be
        // set to '0'.
        //
        // The meaning of this bit for transport stream packets carrying
        // only private data is not defined.

        std::list<TSPacket> & pes = pes_[pkt.pid_];
#if 1
        if (!pes.empty())
        {
          // do something with the previous packets:
          consume(pkt.pid_, pes);
          pes.clear();
        }
#endif
        pes.push_back(pkt);
      }
      else if (yae::has(pes_, uint16_t(pkt.pid_)))
      {
        std::list<TSPacket> & pes = pes_[pkt.pid_];
        if (!pes.empty())
        {
          pes.push_back(pkt);
        }
      }
    }
  }
}