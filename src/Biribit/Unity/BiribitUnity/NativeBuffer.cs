using System.Runtime.InteropServices;
using System.Collections;
using System;

public class NativeBuffer
{
	public uint Length = 0;
	public byte[] data = null;
	public IntPtr ptr = IntPtr.Zero;

	public int GetSize()
	{
		if (data != null)
			return data.Length;

		return 0;
	}

	public void Free()
	{
		if (data != null)
		{
			Marshal.FreeHGlobal(ptr);
			data = null;
		}
	}

	public void Ensure(uint size, bool alloc = true)
	{
		if (size > GetSize())
		{
			if (alloc)
			{
				Free();
				ptr = Marshal.AllocHGlobal((int) size);
			}

			data = new byte[size];
		}
	}
}