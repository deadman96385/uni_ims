/* ezxml.c
 *
 * Copyright 2004-2006 Aaron Voisine <aaron@voisine.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#undef EZXML_NOMMAP

#include "ezxml.h"


// returns the first child tag with the given name or NULL if not found
ezxml_t ezxml_child(ezxml_t xml, const char *name)
{

    return (ezxml_t)NULL;
}

// returns the Nth tag with the same name in the same subsection or NULL if not
// found
ezxml_t ezxml_idx(ezxml_t xml, int idx)
{
    return (ezxml_t)NULL;
}

// returns the value of the requested tag attribute or NULL if not found
const char *ezxml_attr(ezxml_t xml, const char *attr)
{
    return NULL;
}

// same as ezxml_get but takes an already initialized va_list
ezxml_t ezxml_vget(ezxml_t xml, va_list ap)
{
    return (ezxml_t)NULL;
}

// Traverses the xml tree to retrieve a specific subtag. Takes a variable
// length list of tag names and indexes. The argument list must be terminated
// by either an index of -1 or an empty string tag name. Example: 
// title = ezxml_get(library, "shelf", 0, "book", 2, "title", -1);
// This retrieves the title of the 3rd book on the 1st shelf of library.
// Returns NULL if not found.
ezxml_t ezxml_get(ezxml_t xml, ...)
{
    return (ezxml_t)NULL;
}

// returns a null terminated array of processing instructions for the given
// target
const char **ezxml_pi(ezxml_t xml, const char *target)
{
    
    return (const char **)(NULL);
}



// Recursively decodes entity and character references and normalizes new lines
// ent is a null terminated array of alternating entity names and values. set t
// to '&' for general entity decoding, '%' for parameter entity decoding, 'c'
// for cdata sections, ' ' for attribute normalization, or '*' for non-cdata
// attribute normalization. Returns s, or if the decoded string is longer than
// s, returns a malloced string that must be freed.
char *ezxml_decode(char *s, char **ent, char t)
{
    
    return NULL;
}


// checks for circular entity references, returns non-zero if no circular
// references are found, zero otherwise
int ezxml_ent_ok(char *name, char *s, char **ent)
{
    return 0;
}



// Converts a UTF-16 string to UTF-8. Returns a new string that must be freed
// or NULL if no conversion was needed.
char *ezxml_str2utf8(char **s, size_t *len)
{
    return NULL;
}

// frees a tag attribute list
void ezxml_free_attr(char **attr) {
    
}

// parse the given xml string and return an ezxml structure
ezxml_t ezxml_parse_str(char *s, size_t len)
{
    return (ezxml_t)NULL;
}

// Opens an xml file and loads the content into a buffer.
// This is useful if you need to open an xml file and pass
// the contents of the xml document around before you
// actually need to parse it.
// ATTENTION!!  You must free the buffer pointed to in the "s"
// parameter when you are done.
// Returns 0 if successful.  -1 if the file cold not be found.
int ezxml_alloc_str(const char *file, char **str, int *strlen)
{
    
    return (0);
}

// Wrapper for ezxml_parse_str() that accepts a file stream. Reads the entire
// stream into memory and then parses it. For xml files, use ezxml_parse_file()
// or ezxml_parse_fd()
ezxml_t ezxml_parse_fp(FILE *fp)
{
    return (ezxml_t)NULL;
}


// a wrapper for ezxml_parse_fd that accepts a file name
ezxml_t ezxml_parse_file(const char *file)
{
    return (ezxml_t)NULL;
}

// Encodes ampersand sequences appending the results to *dst, reallocating *dst
// if length excedes max. a is non-zero for attribute encoding. Returns *dst
char *ezxml_ampencode(const char *s, size_t len, char **dst, size_t *dlen,
                      size_t *max, short a)
{
    
    return NULL;
}

// Recursively converts each tag to xml appending it to *s. Reallocates *s if
// its length excedes max. start is the location of the previous tag in the
// parent tag's character content. Returns *s.
char *ezxml_toxml_r(ezxml_t xml, char **s, size_t *len, size_t *max,
                    size_t start, char ***attr)
{
    return NULL;
}

// Converts an ezxml structure back to xml. Returns a string of xml data that
// must be freed.
char *ezxml_toxml(ezxml_t xml)
{
    return NULL;
}

// free the memory allocated for the ezxml structure
void ezxml_free(ezxml_t xml)
{
    
}

// returns a new empty ezxml structure with the given root tag name
ezxml_t ezxml_new(const char *name)
{
    return (ezxml_t)NULL;
}

// inserts an existing tag into an ezxml structure
ezxml_t ezxml_insert(ezxml_t xml, ezxml_t dest, size_t off)
{
    return (ezxml_t)NULL;
}

// Adds a child tag. off is the offset of the child tag relative to the start
// of the parent tag's character content. Returns the child tag.
ezxml_t ezxml_add_child(ezxml_t xml, const char *name, size_t off)
{
    return (ezxml_t)NULL;
}

// sets the character content for the given tag and returns the tag
ezxml_t ezxml_set_txt(ezxml_t xml, const char *txt)
{
    return (ezxml_t)NULL;
}

// Sets the given tag attribute or adds a new attribute if not found. A value
// of NULL will remove the specified attribute. Returns the tag given.
ezxml_t ezxml_set_attr(ezxml_t xml, const char *name, const char *value)
{
    return (ezxml_t)NULL;
}

// sets a flag for the given tag and returns the tag
ezxml_t ezxml_set_flag(ezxml_t xml, short flag)
{
    if (xml) xml->flags |= flag;
    return xml;
}

// removes a tag along with its subtags without freeing its memory
ezxml_t ezxml_cut(ezxml_t xml)
{
    ezxml_t cur;

    if (! xml) return NULL; // nothing to do
    if (xml->next) xml->next->sibling = xml->sibling; // patch sibling list

    if (xml->parent) { // not root tag
        cur = xml->parent->child; // find head of subtag list
        if (cur == xml) xml->parent->child = xml->ordered; // first subtag
        else { // not first subtag
            while (cur->ordered != xml) cur = cur->ordered;
            cur->ordered = cur->ordered->ordered; // patch ordered list

            cur = xml->parent->child; // go back to head of subtag list
            if (strcmp(cur->name, xml->name)) { // not in first sibling list
                while (strcmp(cur->sibling->name, xml->name))
                    cur = cur->sibling;
                if (cur->sibling == xml) { // first of a sibling list
                    cur->sibling = (xml->next) ? xml->next
                                               : cur->sibling->sibling;
                }
                else cur = cur->sibling; // not first of a sibling list
            }

            while (cur->next && cur->next != xml) cur = cur->next;
            if (cur->next) cur->next = cur->next->next; // patch next list
        }        
    }
    xml->ordered = xml->sibling = xml->next = NULL;
    return xml;
}

