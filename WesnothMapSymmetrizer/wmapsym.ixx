module;

#include <string>
#include <regex>
#include <vector>
#include <array>
#include <fstream>
#include <numbers>

export module wmapsym;




export namespace WMapSym
{

	struct Coords
	{
		int X, Y;
	};

	class Tile
	{
	public:
		explicit Tile(const std::string& type)
			: _Type(std::regex_replace(type, std::regex{ "^\\s*(.-)\\s*$" }, "$1"))
		{
		}

		int GetPlayer() const
		{
			try
			{
				return std::stoi(_Type);
			}
			catch (std::invalid_argument&)
			{
				return 0;
			}
		}
		
		void SetPlayer(int player)
		{
			_Type = std::regex_replace(_Type, std::regex{ "^\\s*\\d+" }, std::to_string(player));
		}

		const std::string& GetType() const
		{
			return _Type;
		}

	private:
		std::string _Type;
	};
	
	class WesnothMap
	{
	public:
		explicit WesnothMap(std::ifstream& stream)
		{
			std::regex tile_regex{ "[^\\,\\n\\r]+" };
			std::string row;
			int rows_n = 0;
			while (std::getline(stream, row))
			{
				auto begin = std::sregex_iterator(row.begin(), row.end(), tile_regex);
				auto end = std::sregex_iterator();
				for (std::sregex_iterator i = begin; i != end; ++i)
					_Tiles.emplace_back(i->str());

				if (!_Size.X)
					_Size.X = static_cast<int>(_Tiles.size());

				rows_n++;
			}
			_Size.Y = rows_n;
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
			int index = y * _Size.X + x;
			return _Tiles.at(index);
		}

		bool IsValidPos(Coords pos) const
		{
			auto [x, y] = pos;
			return x >= 0 && x < _Size.X && y >= 0 && y < _Size.Y;
		}

		Coords GetSize() const
		{
			return _Size;
		}

		void WriteToFile(std::ofstream& stream) const
		{
			Coords cur_pos = { 0, 0 };
			for (const auto& tile : _Tiles)
			{
				stream << tile.GetType();
				++cur_pos.X;
				
				if (cur_pos.X >= _Size.X)
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
		std::vector<Tile> _Tiles{};
		Coords _Size{};
	};


	class Simple4PlayersSymmetrizer
	{
	public:
		explicit Simple4PlayersSymmetrizer(const WesnothMap& map, int angleDeg)
			: _Map{ map }
			, _AngleRad{ angleDeg * DEG_TO_RAD }
		{
			_Symmetrize();
		}

		WesnothMap& GetSymmetrizedMap()
		{
			return _Map;
		}

	private:
		WesnothMap _Map;
		double _AngleRad;

		static constexpr double DEG_TO_RAD = std::numbers::pi / 180;

		void _Symmetrize()
		{
			auto size = _Map.GetSize();

			for (int y = 0; y < size.Y; y++)
			for (int x = 0; x < size.X; x++)
			{
				Coords pos = { x, y };
				if (_TileCoordCondition(pos))
					_CopyTransformedTile(pos);
			}
		}

		bool _TileCoordCondition(Coords pos)
		{
			double dx = pos.X - (_Map.GetSize().X - 1) * 0.5;
			double dy = pos.Y - (_Map.GetSize().Y - 1) * 0.5;

			double angle = -_AngleRad;
			double cos = std::cos(angle);
			double sin = std::sin(angle);
			double x2 = dx * cos - dy * sin;
			double y2 = dx * sin + dy * cos;
			
			return x2 >= 0 && y2 >= 0;
		}

		void _CopyTransformedTile(Coords pos)
		{
			Tile& tile = _Map.GetTile(pos);
			auto transformed = _TransformedPositions(pos);

			if (tile.GetPlayer())
				tile.SetPlayer(1);

			int player_zone_i = 2;
			for (const auto& coords : transformed)
			{
				Tile& dup_tile = _Map.GetTile(coords);
				dup_tile = tile;

				if (dup_tile.GetPlayer())
					dup_tile.SetPlayer(player_zone_i);

				player_zone_i++;
			}
		}

		std::array<Coords, 3> _TransformedPositions(Coords pos) const
		{
			auto [x, y] = pos;
			int tx = _Map.GetSize().X - x - 1;
			int ty = _Map.GetSize().Y - y - 1;
			
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




