namespace TmsControlPanel;

/// <summary>Bir netif üzerindeki tek UDP soketi (nth = firmware'deki 1 tabanlı sıra).</summary>
public record SocketInfo(int Nth, int Port);

/// <summary>Karttaki bir ağ arayüzü (Index = firmware'deki 1 tabanlı sıra).</summary>
public record NetifInfo(int Index, string Ip, string Mask, string Gw, List<SocketInfo> Sockets);

/// <summary>Protokol veri satırlarını ("#NETIF ...", "#SOCK ...") nesnelere çevirir.</summary>
public static class Protocol
{
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
