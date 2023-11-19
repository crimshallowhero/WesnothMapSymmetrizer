module;

#include <string>
#include <regex>
#include <vector>
#include <array>
#include <fstream>
#include <numbers>

export module wmapsym;



const std::regex REGEX_TRIM{ "^\\s*(.-)\\s*$" };
std::string StringTrim(const std::string& s)
{
    return std::regex_replace(s, REGEX_TRIM, "$1");
}



export namespace wmapsym
{

    using Player = int;

    struct Coords
    {
        int X, Y;
    };

    class Tile
    {
    public:
        explicit Tile(const std::string& type)
            : type_{ StringTrim(type) }
        {
        }

        std::optional<Player> GetPlayer() const
        {
            try
            {
                return std::stoi(type_);
            }
            catch (std::invalid_argument&)
            {
                return std::nullopt;
            }
        }
        
        void SetPlayer(Player player)
        {
            type_ = std::regex_replace(type_, REGEX_PLAYER, std::to_string(player));
        }

        const std::string& GetType() const
        {
            return type_;
        }

    private:
        std::string type_;

        static inline const std::regex REGEX_PLAYER{ "^\\s*\\d+" };
    };


    class WesnothMap
    {
    public:
        explicit WesnothMap(std::ifstream& stream)
        {
            int rows_n = 0;
            for (std::string row; std::getline(stream, row);)
            {
                for (auto i = std::sregex_iterator(row.begin(), row.end(), REGEX_TILE), end = std::sregex_iterator(); i != end; ++i)
                    tiles_.emplace_back(i->str());

                if (!size_.X)
                    size_.X = static_cast<int>(tiles_.size());

                rows_n++;
            }
            size_.Y = rows_n;
        }

        //// Fails to compile due to internal compiler error
        //auto& GetTile(this auto& self, Coords pos)
        //{
        //	if (!self.IsValidPos(pos))
        //		throw std::out_of_range{ "coords out of range" };
        //
        //	auto [x, y] = pos;
        //	int index = y * self._Size.X + x;
        //	return self._Tiles.at(index);
        //}

        Tile& GetTile(Coords pos)
        {
            if (!IsValidPos(pos))
                throw std::out_of_range{ "coords out of range" };
        
            auto [x, y] = pos;
            int index = y * size_.X + x;
            return tiles_.at(index);
        }

        bool IsValidPos(Coords pos) const
        {
            auto [x, y] = pos;
            return x >= 0 && x < size_.X && y >= 0 && y < size_.Y;
        }

        Coords GetSize() const
        {
            return size_;
        }

        void WriteToFile(std::ofstream& stream) const
        {
            Coords cur_pos = { 0, 0 };
            for (const auto& tile : tiles_)
            {
                stream << tile.GetType();
                ++cur_pos.X;
                
                if (cur_pos.X >= size_.X)
                {
                    stream << "\n";
                    cur_pos.X = 0;
                    ++cur_pos.Y;
                }
                else
                    stream << ", ";
            }
        }

    private:
        std::vector<Tile> tiles_{};
        Coords size_{};

        static inline const std::regex REGEX_TILE{ "[^\\,\\n\\r]+" };
    };


    class Simple4PlayersSymmetrizer
    {
    public:
        explicit Simple4PlayersSymmetrizer(const WesnothMap& map, int angle_deg)
            : map_{ map }
            , angle_rad_{ angle_deg * DEG_TO_RAD }
        {
            Symmetrize();
        }

        WesnothMap& GetSymmetrizedMap()
        {
            return map_;
        }

    private:
        WesnothMap map_;
        double angle_rad_;

        static constexpr double DEG_TO_RAD = std::numbers::pi / 180;

        void Symmetrize()
        {
            auto size = map_.GetSize();

            for (int y = 0; y < size.Y; y++)
            for (int x = 0; x < size.X; x++)
            {
                Coords pos = { x, y };
                if (TileCoordCondition(pos))
                    CopyTransformedTile(pos);
            }
        }

        bool TileCoordCondition(Coords pos) const
        {
            double dx = pos.X - (map_.GetSize().X - 1) * 0.5;
            double dy = pos.Y - (map_.GetSize().Y - 1) * 0.5;

            double angle = -angle_rad_;
            double cos = std::cos(angle);
            double sin = std::sin(angle);
            double x2 = dx * cos - dy * sin;
            double y2 = dx * sin + dy * cos;
            
            return x2 >= 0 && y2 >= 0;
        }

        void CopyTransformedTile(Coords pos)
        {
            Tile& tile = map_.GetTile(pos);

            if (tile.GetPlayer())
                tile.SetPlayer(Player{ 1 });

            int player_zone_i = 2;

            auto transformed = TransformedPositions(pos);
            for (const auto& coords : transformed)
            {
                Tile& dup_tile = map_.GetTile(coords);

                dup_tile = tile;

                if (dup_tile.GetPlayer())
                    dup_tile.SetPlayer(Player{ player_zone_i });

                player_zone_i++;
            }
        }

        std::array<Coords, 3> TransformedPositions(Coords pos) const
        {
            auto [x, y] = pos;
            auto [sx, sy] = map_.GetSize();
            int tx = sx - x - 1;
            int ty = sy - y - 1;
            
            return
            {
                {
                    {tx,  y},
                    { x, ty},
                    {tx, ty},
                }
            };
        }
    };


}




