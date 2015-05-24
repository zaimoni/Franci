#ifdef ZAIMONI_AUTOPTR_HPP 
#ifdef ZAIMONI_CSTDIO 
#ifndef ZAIMONI_STL_BITS_CSTDIO_AUTOPTR_HPP
#define ZAIMONI_STL_BITS_CSTDIO_AUTOPTR_HPP 1

namespace zaimoni {

template<class T>
inline typename boost::enable_if<boost::has_trivial_assign<T>, bool>::type
GetBinaryFileImage(const char* const Filename,_meta_autoarray_ptr<T>& buf)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/10/2004
	assert(Filename && *Filename);
	assert(buf.empty());

	FILE* TargetFile = fopen(Filename, "rb");
	if (!TargetFile) return false;

	// Want to know the file length *BEFORE* inhaling it
	long ScriptLength = get_filelength(TargetFile);
	if (-1==ScriptLength)
		{
		fclose(TargetFile);
		return false;
		}

	assert(0==ScriptLength%sizeof(T));
	size_t ActuallyRead = 0;
	if (0<ScriptLength)
		{
		if (!buf.Resize(ScriptLength)) return false;
		ActuallyRead = fread(buf,sizeof(T),ScriptLength/sizeof(T),TargetFile);
		}

	fclose(TargetFile);
	return ActuallyRead==ScriptLength/sizeof(T);
}

}	// end namespace zaimoni

#endif
#endif
#endif
