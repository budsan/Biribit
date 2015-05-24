using UnityEngine;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System;
using Silver;


public class BiribitManager : MonoBehaviour
{
	static private BiribitManager m_managerInstance = null;
	static private BiribitClient m_instance = null;
	static public BiribitClient Instance
	{
		get
		{
			if (m_managerInstance == null)
			{
				GameObject manager = new GameObject("BiribitManager");
				manager.hideFlags = HideFlags.HideInHierarchy;
				manager.AddComponent<BiribitManager>();
			}

			if (m_instance == null)
			{
				m_instance = new BiribitClient();
			}
				
			return m_instance;
		}
	}

	protected partial class NativeMethods
	{
		const string DllNameClient = "BiribitClient";

		[DllImport(DllNameClient, EntryPoint = "BiribitClientClean")]
		public static extern bool Clean();

		[DllImport(DllNameClient, EntryPoint = "BiribitClientAddLogCallback")]
		public static extern bool ClientAddLogCallback(IntPtr func);

		[DllImport(DllNameClient, EntryPoint = "BiribitClientDelLogCallback")]
		public static extern bool ClientDelLogCallback(IntPtr func);
	}

	private delegate void ClientLogCallbackDelegate(string msg);
	private ClientLogCallbackDelegate m_clientCallback = null;
	private IntPtr m_clientCallbackPtr;

	private void DebugLog(string msg)
	{
		Debug.Log(msg);
	}

	private void Awake()
	{
		NativeMethods.Clean();
		/*
		m_clientCallback = new ClientLogCallbackDelegate(DebugLog);
		m_clientCallbackPtr = Marshal.GetFunctionPointerForDelegate(m_clientCallback);
		NativeMethods.ClientAddLogCallback(m_clientCallbackPtr);

		if (m_managerInstance == null)
			m_managerInstance = this;
		else
			Destroy(this);
		 */
	}

	private void OnDestroy()
	{
		/*
		if (m_managerInstance == this)
			m_managerInstance = null;

		NativeMethods.ClientDelLogCallback(m_clientCallbackPtr);
		 */
	}

	private void OnApplicationQuit()
	{
		NativeMethods.Clean();
	}
}

