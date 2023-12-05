/* SPDX-License-Identifier: MIT */
/*
 * Copyright 2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#ifndef __DCN32_FPU_H__
#define __DCN32_FPU_H__

#include "clk_mgr_internal.h"

#define DCN3_2_DEFAULT_DET_SIZE 256
#define DCN3_2_MAX_DET_SIZE 1152
#define DCN3_2_MIN_DET_SIZE 128
#define DCN3_2_MIN_COMPBUF_SIZE_KB 128

void dcn32_build_wm_range_table_fpu(struct clk_mgr_internal *clk_mgr);

void dcn32_helper_populate_phantom_dlg_params(struct dc *dc,
					      struct dc_state *context,
					      display_e2e_pipe_params_st *pipes,
					      int pipe_cnt);

bool dcn32_predict_pipe_split(struct dc_state *context,
			      display_pipe_params_st pipe,
			      int index);

void insert_entry_into_table_sorted(struct _vcs_dpi_voltage_scaling_st *table,
				    unsigned int *num_entries,
				    struct _vcs_dpi_voltage_scaling_st *entry);

void dcn32_set_phantom_stream_timing(struct dc *dc,
				     struct dc_state *context,
				     struct pipe_ctx *ref_pipe,
				     struct dc_stream_state *phantom_stream,
				     display_e2e_pipe_params_st *pipes,
				     unsigned int pipe_cnt,
				     unsigned int dc_pipe_idx);

bool dcn32_internal_validate_bw(struct dc *dc,
				struct dc_state *context,
				display_e2e_pipe_params_st *pipes,
				int *pipe_cnt_out,
				int *vlevel_out,
				bool fast_validate);

void dcn32_calculate_wm_and_dlg_fpu(struct dc *dc, struct dc_state *context,
				display_e2e_pipe_params_st *pipes,
				int pipe_cnt,
				int vlevel);

void dcn32_update_bw_bounding_box_fpu(struct dc *dc, struct clk_bw_params *bw_params);

int dcn32_find_dummy_latency_index_for_fw_based_mclk_switch(struct dc *dc,
							    struct dc_state *context,
							    display_e2e_pipe_params_st *pipes,
							    int pipe_cnt,
							    int vlevel);

void dcn32_patch_dpm_table(struct clk_bw_params *bw_params);

#endif