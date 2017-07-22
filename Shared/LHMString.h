#pragma once

//TODO: Static string when capacity < 0?
//TODO: String views or slices (they need to know they don't own the string. Actually, the 'static string' concept will work here I think).
//TODO: Maybe compress them to u16? Really should need strings that long, eh?

/* NOTES:
 * - Null terminated for C compatibility
 */

struct String
{
	u32  length;
	u32  capacity;
	c16* data;

	inline operator c16*() { return data; }
};