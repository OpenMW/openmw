#include "cellcoordinates.hpp"

#include <cmath>

#include <ostream>
#include <sstream>

#include <components/esm/loadland.hpp>
#include <components/misc/constants.hpp>

CSMWorld::CellCoordinates::CellCoordinates() : mX (0), mY (0) {}

CSMWorld::CellCoordinates::CellCoordinates (int x, int y) : mX (x), mY (y) {}

CSMWorld::CellCoordinates::CellCoordinates (const std::pair<int, int>& coordinates)
: mX (coordinates.first), mY (coordinates.second) {}

int CSMWorld::CellCoordinates::getX() const
{
    return mX;
}

int CSMWorld::CellCoordinates::getY() const
{
    return mY;
}

CSMWorld::CellCoordinates CSMWorld::CellCoordinates::move (int x, int y) const
{
    return CellCoordinates (mX + x, mY + y);
}

std::string CSMWorld::CellCoordinates::getId (const std::string& worldspace) const
{
    // we ignore the worldspace for now, since there is only one (will change in 1.1)
    return generateId(mX, mY);
}

std::string CSMWorld::CellCoordinates::generateId (int x, int y)
{
    std::string cellId = "#" + std::to_string(x) + " " + std::to_string(y);
    return cellId;
}

bool CSMWorld::CellCoordinates::isExteriorCell (const std::string& id)
{
    return (!id.empty() && id[0]=='#');
}

std::pair<CSMWorld::CellCoordinates, bool> CSMWorld::CellCoordinates::fromId (
    const std::string& id)
{
    // no worldspace for now, needs to be changed for 1.1
    if (isExteriorCell(id))
    {
        int x, y;
        char ignore;

        std::istringstream stream (id);
        if (stream >> ignore >> x >> y)
            return std::make_pair (CellCoordinates (x, y), true);
    }

    return std::make_pair (CellCoordinates(), false);
}

std::pair<int, int> CSMWorld::CellCoordinates::coordinatesToCellIndex (float x, float y)
{
    return std::make_pair (std::floor (x / Constants::CellSizeInUnits), std::floor (y / Constants::CellSizeInUnits));
}

std::pair<int, int> CSMWorld::CellCoordinates::toTextureCoords(const osg::Vec3d& worldPos)
{
    const auto xd = static_cast<float>(worldPos.x() * ESM::Land::LAND_TEXTURE_SIZE / ESM::Land::REAL_SIZE - 0.25f);
    const auto yd = static_cast<float>(worldPos.y() * ESM::Land::LAND_TEXTURE_SIZE / ESM::Land::REAL_SIZE + 0.25f);

    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    return std::make_pair(x, y);
}

std::pair<int, int> CSMWorld::CellCoordinates::toVertexCoords(const osg::Vec3d& worldPos)
{
    const auto xd = static_cast<float>(worldPos.x() * (ESM::Land::LAND_SIZE - 1) / ESM::Land::REAL_SIZE + 0.5f);
    const auto yd = static_cast<float>(worldPos.y() * (ESM::Land::LAND_SIZE - 1) / ESM::Land::REAL_SIZE + 0.5f);

    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    return std::make_pair(x, y);
}

float CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(int textureGlobal)
{
    return ESM::Land::REAL_SIZE * (static_cast<float>(textureGlobal) + 0.25f) / ESM::Land::LAND_TEXTURE_SIZE;
}

float CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(int textureGlobal)
{
    return ESM::Land::REAL_SIZE * (static_cast<float>(textureGlobal) - 0.25f) / ESM::Land::LAND_TEXTURE_SIZE;
}

float CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(int vertexGlobal)
{
    return ESM::Land::REAL_SIZE * static_cast<float>(vertexGlobal) / (ESM::Land::LAND_SIZE - 1);
}

int CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(int vertexGlobal)
{
    return static_cast<int>(vertexGlobal - std::floor(static_cast<float>(vertexGlobal) / (ESM::Land::LAND_SIZE - 1)) * (ESM::Land::LAND_SIZE - 1));
}

std::string CSMWorld::CellCoordinates::textureGlobalToCellId(const std::pair<int, int>& textureGlobal)
{
    int x = std::floor(static_cast<float>(textureGlobal.first) / ESM::Land::LAND_TEXTURE_SIZE);
    int y = std::floor(static_cast<float>(textureGlobal.second) / ESM::Land::LAND_TEXTURE_SIZE);
    return generateId(x, y);
}

std::string CSMWorld::CellCoordinates::vertexGlobalToCellId(const std::pair<int, int>& vertexGlobal)
{
    int x = std::floor(static_cast<float>(vertexGlobal.first) / (ESM::Land::LAND_SIZE - 1));
    int y = std::floor(static_cast<float>(vertexGlobal.second) / (ESM::Land::LAND_SIZE - 1));
    return generateId(x, y);
}

bool CSMWorld::operator== (const CellCoordinates& left, const CellCoordinates& right)
{
    return left.getX()==right.getX() && left.getY()==right.getY();
}

bool CSMWorld::operator!= (const CellCoordinates& left, const CellCoordinates& right)
{
    return !(left==right);
}

bool CSMWorld::operator< (const CellCoordinates& left, const CellCoordinates& right)
{
    if (left.getX()<right.getX())
        return true;

    if (left.getX()>right.getX())
        return false;

    return left.getY()<right.getY();
}

std::ostream& CSMWorld::operator<< (std::ostream& stream, const CellCoordinates& coordiantes)
{
    return stream << coordiantes.getX() << ", " << coordiantes.getY();
}
