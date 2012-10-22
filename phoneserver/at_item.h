
/*
 *
 * at_item.h: at item implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */
struct at_item {

    /***    User explicit entries    ***/
	void *adapter;		/*## ptr to adapter */
	enum mode;		/*## attribute mode */
	char tag_str[];		/*## attribute tag_str */
};
