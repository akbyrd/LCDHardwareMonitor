﻿namespace LCDHardwareMonitor
{
	class TreeModel
	{
		public Node Root { get; private set; }

		public TreeModel ()
		{
			Root = new Node();
		}
	}
}
