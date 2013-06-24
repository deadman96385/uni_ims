
#include "vp8_yv12config.h"
#include "vp8_mode.h"
#include "vp8dec_reg.h"
#include "vp8dec.h"

void vp8_swap_yv12_buffer(YV12_BUFFER_CONFIG *new_frame, YV12_BUFFER_CONFIG *last_frame)
{
    unsigned char *temp;
	unsigned int tmp;

    temp = last_frame->buffer_alloc;
    last_frame->buffer_alloc = new_frame->buffer_alloc;
    new_frame->buffer_alloc = temp;

    temp = last_frame->y_buffer;
    last_frame->y_buffer = new_frame->y_buffer;
    new_frame->y_buffer = temp;

    temp = last_frame->u_buffer;
    last_frame->u_buffer = new_frame->u_buffer;
    new_frame->u_buffer = temp;

    temp = last_frame->v_buffer;
    last_frame->v_buffer = new_frame->v_buffer;
    new_frame->v_buffer = temp;

	tmp = last_frame->addr_idx;
    last_frame->addr_idx = new_frame->addr_idx;
    new_frame->addr_idx = tmp;
}


void vp8_copy_yv12_buffer(VPXHandle *vpxHandle, VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame, YV12_BUFFER_CONFIG *dst_frame)
{
	// Check if no one is referencing
//	cm->ref_count[dst_frame->addr_idx] -= 1;
//	if (cm->ref_count[dst_frame->addr_idx] <= 0)
//	{
//		if(dst_frame->pBufferHeader != NULL )
//		{
//			OR_VSP_UNBIND(dst_frame->pBufferHeader);
//		}
//		cm->buffer_pool[cm->buffer_count++] = *dst_frame;
//		cm->ref_count[dst_frame->addr_idx] = 0;
//	}

	{
		int buffer_index;

		// Bind 
		if(src_frame->pBufferHeader != NULL)
		{
			for(buffer_index = 0; buffer_index <4; buffer_index ++)
			{
				if(cm->buffer_pool[buffer_index] ==  src_frame->pBufferHeader)
				{
					break;
				}
			}

			if(buffer_index <4)
			{
				if(cm->ref_count[buffer_index] ==0)
					OR_VSP_BIND(vpxHandle,src_frame->pBufferHeader);

				cm->ref_count[buffer_index] ++;
			}
		}
			
		//UnBind
		if(dst_frame->pBufferHeader != NULL)
		{
			for(buffer_index = 0; buffer_index <4; buffer_index ++)
			{
				if(cm->buffer_pool[buffer_index] ==  dst_frame->pBufferHeader)
				{
					break;
				}
			}

			
			if(buffer_index <4)
			{
				cm->ref_count[buffer_index] --;
				
				if(cm->ref_count[buffer_index] ==0)
				{
						OR_VSP_UNBIND(vpxHandle,dst_frame->pBufferHeader);
						cm->buffer_pool[buffer_index] = NULL;
				}					
			}
		}	
		
	}



	dst_frame->buffer_alloc = src_frame->buffer_alloc;
	dst_frame->y_buffer = src_frame->y_buffer;
	dst_frame->u_buffer = src_frame->u_buffer;
	dst_frame->v_buffer = src_frame->v_buffer;
	dst_frame->addr_idx = src_frame->addr_idx;

	dst_frame->y_buffer_virtual = src_frame->y_buffer_virtual;
	dst_frame->u_buffer_virtual = src_frame->u_buffer_virtual;

	dst_frame->pBufferHeader = src_frame->pBufferHeader;
	
//	cm->ref_count[src_frame->addr_idx] += 1;
}

#if 0
void vp8_check_yv12_buffer(VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame)
{
	if (cm->ref_count[src_frame->addr_idx] > 1)
	{
		SCI_ASSERT(cm->buffer_count>0);
		cm->ref_count[src_frame->addr_idx] -= 1;
		*src_frame = cm->buffer_pool[cm->buffer_count-1];
		cm->ref_count[src_frame->addr_idx] = 1;
		cm->buffer_count--;
		SCI_ASSERT(cm->buffer_count>=0);
	}
}
#endif 