import Types.NetworkFrame;
import Types.TileMap;
import haxe.io.Bytes;
import haxe.io.BytesBuffer;
#if sys
import sys.net.Host;
import sys.net.Host;
import sys.net.Socket;
import sys.net.UdpSocket;
#end

// the job of this client is to retrieve a map informations before the beginning of the game
class TCPClient
{
	var sock(default, null):Socket;

	public var files:haxe.ds.Vector<Bytes>;

	public var cur_file:Int = -1;
	public var n_files:Int = -1;
	public var total_size:Int = -1;
	public var read_so_far:Int = -1;
	public var connected:Bool = false;
	public var done:Bool = false;

	public function new() {}

	public function download(address:String, port:Int, n_files:Int)
	{
		#if debug
		if (connected)
		{
			trace("TCP CONNECT WHILE ALREADY CONNECTED");
		}
		#end

		this.n_files = n_files;
		this.cur_file = 0;
		files = new haxe.ds.Vector<Bytes>(n_files);

		sock = new Socket();
		var host = new Host(address);
		sock.connect(host, port);
		connected = true;
		done = false;
		sock.setTimeout(0.01);

		trace('connected to $address:$port');
		trace('downloading $n_files files');
	}

	public function write(frame:NetworkFrame)
	{
		var bytes = Utils.bytesOfNetworkFrame(frame);
		return sock.output.writeBytes(bytes, 0, bytes.length);
	}

	public function update()
	{
		try
		{
			if (total_size < 0)
			{
				var local_buffer = Bytes.alloc(5);
				var actually_read = sock.input.readBytes(local_buffer, 0, 5);

				var reader = new ByteReader(local_buffer);

				var opcode = reader.readInt8();
				var expected_length = reader.readInt32();

				#if debug
				if (opcode != Config.OP_BINARY)
					trace("TCP sending non binary file !!");
				#end

				total_size = expected_length;
				read_so_far = 0;
				files[cur_file] = Bytes.alloc(total_size);

				trace("downloading file " + cur_file + " of size " + expected_length);
			}

			// now total_size is > 0

			var remaining = total_size - read_so_far;
			var size = remaining > 1024 ? 1024 : remaining;
			var actually_read = sock.input.readBytes(files[cur_file], read_so_far, size);
			read_so_far += actually_read;

			if (read_so_far >= total_size)
			{
				cur_file++;
				total_size = -1;
				trace("done");

				if (cur_file >= n_files)
				{
					done = true;
					sock.close();
					trace("tcp server closed");
				}
			}
		}
		catch (Broken) {};
	}
}
