module;

#include <string>
#include <regex>
#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <numbers>
#include <optional>
#include <concepts>
#include <ranges>

export module wmapsym;


const std::regex REGEX_TRIM{"^\\s*(.-)\\s*$"};

std::string StringTrim(const std::string& s) {
	return std::regex_replace(s, REGEX_TRIM, "$1");
}


constexpr double DEG_TO_RAD = std::numbers::pi / 180;
double DegToRad(double deg) { return deg * DEG_TO_RAD; }


export namespace wmapsym {
	using Player = int;

	struct Coords {
		int X, Y;
	};

	class Tile {
	public:
		explicit Tile(const std::string& type)
			: type_{ StringTrim(type) } { }

		std::optional<Player> GetPlayer() const {
			try {
				return std::stoi(type_);
			} catch (std::invalid_argument&) {
				return std::nullopt;
			}
		}

		void SetPlayer(Player player) {
			type_ = std::regex_replace(type_, REGEX_PLAYER, std::to_string(player));
		}

		const std::string& GetType() const {
			return type_;
		}

		void AddPostfix(const std::string& postfix) {
			if (auto i = type_.find("^"); i != std::string::npos)
				type_.erase(i);

			type_ += postfix;
		}

	private:
		std::string type_;

		static inline const std::regex REGEX_PLAYER{"^\\s*\\d+"};
	};


	class WesnothMap {
	public:
		explicit WesnothMap(std::ifstream& stream) {
			int rows_n = 0;
			for (std::string row; std::getline(stream, row);) {
				for (auto i = std::sregex_iterator(row.begin(), row.end(), REGEX_TILE), end = std::sregex_iterator();
				     i != end;
				     ++i)
					tiles_.emplace_back(i->str());

				if (size_.X == 0)
					size_.X = static_cast<int>(tiles_.size());

				rows_n++;
			}
			size_.Y = rows_n;
		}

		auto& GetTile(Coords pos) const { return GetTileImpl(*this, pos); }
		auto& GetTile(Coords pos) { return GetTileImpl(*this, pos); }

		bool IsValidPos(Coords pos) const {
			auto [x, y] = pos;
			return x >= 0 && x < size_.X && y >= 0 && y < size_.Y;
		}

		Coords GetSize() const { return size_; }

		void Resize(Coords new_size) {
			auto new_tiles = std::vector<Tile>(new_size.X * new_size.Y, Tile{"Gg"});

			int size_x = std::min(size_.X, new_size.X);
			int size_y = std::min(size_.Y, new_size.Y);

			for (int y = 0; y < size_y; ++y)
				for (int x = 0; x < size_x; ++x)
					new_tiles.at(y * new_size.X + x) = GetTile({x, y});

			std::swap(tiles_, new_tiles);
			size_ = new_size;
		}

		void WriteToFile(std::ofstream& stream) const {
			Coords cur_pos = {0, 0};
			for (const auto& tile : tiles_) {
				stream << tile.GetType();
				++cur_pos.X;

				if (cur_pos.X >= size_.X) {
					stream << "\n";
					cur_pos.X = 0;
					++cur_pos.Y;
				} else {
					stream << ", ";
				}
			}
		}

	private:
		std::vector<Tile> tiles_{};
		Coords size_{};

		static auto& GetTileImpl(std::common_with<WesnothMap> auto& self, Coords pos) {
			if (!self.IsValidPos(pos))
				throw std::out_of_range{"coords out of range"};

			auto [x, y] = pos;
			int index = y * self.size_.X + x;
			return self.tiles_.at(index);
		}

		static inline const std::regex REGEX_TILE{"[^\\,\\n\\r]+"};
	};


	class Simple4PlayersSymmetrizer {
	public:
		explicit Simple4PlayersSymmetrizer(const WesnothMap& map, int angle_deg)
			: map_{map}
			, angle_deg_{angle_deg % 90 == 0 ? angle_deg % 360 : throw std::out_of_range("angle must be divisible by 90")} {
			auto size = map.GetSize();
			auto rem_x = size.X & 1, rem_y = size.Y & 1;
			if (rem_x == 0 || rem_y == 0)
				map_.Resize({size.X - 1 + rem_x, size.Y - 1 + rem_y});

			Symmetrize();
		}

		WesnothMap& GetSymmetrizedMap() {
			return map_;
		}

	private:
		WesnothMap map_;
		int angle_deg_;

		//static constexpr double DEG_TO_RAD = std::numbers::pi / 180;

		void Symmetrize() {
			auto size = map_.GetSize();

			for (int y = 0; y < size.Y; y++)
				for (int x = 0; x < size.X; x++) {
					Coords pos = {x, y};
					if (TileCoordCondition(pos))
						CopyTransformedTile(pos);
				}

			BlockLowerExtraTiles();
		}

		bool TileCoordCondition(Coords pos) const {
			auto [x, y] = pos;
			auto [sx, sy] = map_.GetSize();

			int dx, dy;

			switch (angle_deg_) {
				case 0:
					dx = 1;
					dy = 1;
					break;
				case 90:
					dx = -1;
					dy = 1;
					break;
				case 180:
					dx = -1;
					dy = -1;
					break;
				case 270:
					dx = 1;
					dy = -1;
					break;
				default: throw std::out_of_range{"angle must be divisible by 90"};
			}

			if (dy < 0)
				y -= x + 1 & 1;

			return
				dx * x >= dx * sx / 2
				&& dy * y > dy * sy / 2 - 1;
		}

		void BlockLowerExtraTiles() {
			int y = 1, start_x = 1;
			for (int x = start_x; x < map_.GetSize().X; x += 2)
				map_.GetTile({x, y}).AddPostfix("^Xo");
		}

		/*bool TileCoordCondition(Coords pos) const
		{
		    double dx = pos.X - (map_.GetSize().X - 1) * 0.5;
		    double dy = pos.Y - (map_.GetSize().Y - 1) * 0.5;

		    double angle = -angle_deg_;
		    double cos = std::cos(angle);
		    double sin = std::sin(angle);
		    double x2 = dx * cos - dy * sin;
		    double y2 = dx * sin + dy * cos;
		    
		    return x2 >= 0 && y2 >= 0;
		}*/

		void CopyTransformedTile(Coords pos) {
			Tile& tile = map_.GetTile(pos);

			if (tile.GetPlayer())
				tile.SetPlayer(Player{1});

			int player_zone_i = 2;

			auto transformed = TransformedPositions(pos);
			for (const auto& coords : transformed) {
				if (!map_.IsValidPos(coords)) continue;

				Tile& dup_tile = map_.GetTile(coords);

				dup_tile = tile;

				if (dup_tile.GetPlayer())
					dup_tile.SetPlayer(Player{player_zone_i});

				player_zone_i++;
			}
		}

		std::array<Coords, 3> TransformedPositions(Coords pos) const {
			auto [x, y] = pos;
			auto [sx, sy] = map_.GetSize();


			int tx = sx - x - 1;
			int ty = sy - y;

			int my = sy / 2;
			if (y > my != ty > my)
				ty -= x + 1 & 1;

			return
			{
				{
					{tx, y},
					{x, ty},
					{tx, ty},
				}
			};
		}
	};
}
