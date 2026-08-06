namespace TmsControlPanel;

/// <summary>Bir netif üzerindeki tek UDP soketi (nth = firmware'deki 1 tabanlı sıra).</summary>
public record SocketInfo(int Nth, int Port);

/// <summary>Karttaki bir ağ arayüzü (Index = firmware'deki 1 tabanlı sıra).</summary>
public record NetifInfo(int Index, string Ip, string Mask, string Gw, List<SocketInfo> Sockets);

/// <summary>Tek bir ping denemesinin sonucu.</summary>
public record PingReply(bool Success, string From, int Seq, int Bytes, int RttMs, int Ttl);

/// <summary>Ağ taramasında bulunan bir cihaz.</summary>
public record ScanHost(string Ip, string Mac);

/// <summary>Protokol veri satırlarını ("#NETIF ...", "#SOCK ...") nesnelere çevirir.</summary>
public static class Protocol
{
    // "#PONG 10.0.0.1 3 32 1 64"  ->  from seq bytes rtt ttl
    // "#TIMEOUT"
    public static PingReply ParsePing(ProtocolResponse resp)
    {
        foreach (string line in resp.DataLines)
        {
            string[] t = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            if (t.Length >= 6 && t[0] == "#PONG")
            {
                int.TryParse(t[2], out int seq);
                int.TryParse(t[3], out int bytes);
                int.TryParse(t[4], out int rtt);
                int.TryParse(t[5], out int ttl);
                return new PingReply(true, t[1], seq, bytes, rtt, ttl);
            }
        }
        return new PingReply(false, "", 0, 0, 0, 0);
    }

    // scan_network'ün bastığı satır: "Host up: 10.0.0.5  MAC 00:1a:2b:3c:4d:5e"
    public static List<ScanHost> ParseScan(ProtocolResponse resp)
    {
        var hosts = new List<ScanHost>();
        foreach (string line in resp.DataLines)
        {
            if (!line.StartsWith("Host up:")) continue;

            string[] t = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            // t = ["Host", "up:", ip, "MAC", mac]
            if (t.Length >= 5)
                hosts.Add(new ScanHost(t[2], t[4]));
        }
        return hosts;
    }

    // "#NETIF 1 10.0.0.10 255.255.255.0 10.0.0.1 socks=2"
    // "#SOCK 1 1 5000"
    public static List<NetifInfo> ParseNetifList(ProtocolResponse resp)
    {
        var result = new List<NetifInfo>();

        foreach (string line in resp.DataLines)
        {
            string[] t = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);

            if (t.Length >= 5 && t[0] == "#NETIF" && int.TryParse(t[1], out int nIdx))
            {
                result.Add(new NetifInfo(nIdx, t[2], t[3], t[4], new List<SocketInfo>()));
            }
            else if (t.Length >= 4 && t[0] == "#SOCK"
                     && int.TryParse(t[1], out int owner)
                     && int.TryParse(t[2], out int nth)
                     && int.TryParse(t[3], out int port))
            {
                result.Find(n => n.Index == owner)?.Sockets.Add(new SocketInfo(nth, port));
            }
            // tanınmayan satırlar sessizce atlanır
        }

        return result;
    }
}
