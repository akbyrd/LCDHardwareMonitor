using System.Collections.Generic;

namespace LCDHardwareMonitor
{
	class Node
	{
		public object Source { get; set; }
		public string Text { get; set; }
		public IList<Node> Nodes { get; private set; }

		public Node ()
		{
			Nodes = new List<Node>();
		}
	}
}
