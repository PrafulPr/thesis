#include "Collimator.hh"

#include "G4Box.hh"
#include "G4ExtrudedSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include <cmath>
#include <vector>

void BuildCollimator(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid
)
{   //Parameters:
    const G4double thickness = 35*mm;
    const G4double holeWidth = 1.50*mm;
    const G4double septum = 0.20*mm;

    const G4double halfX = crystalSolid->GetXHalfLength();
    const G4double halfY = crystalSolid->GetYHalfLength();

    // Crystal is centered at z = 0.
    const G4double centreZ =
        -crystalSolid->GetZHalfLength() - thickness/2;

    auto lead =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Pb"); //material

    // Lead fills the entire plate except the vacuum channels.
    auto blockSolid = new G4Box(
        "CollimatorBlock", halfX, halfY, thickness/2
    );

    auto blockLogical = new G4LogicalVolume(
        blockSolid, lead, "Collimator"
    );

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0, 0, centreZ),
        blockLogical,
        "Collimator",
        worldLogical,
        false,
        0,
        true
    );

    // Make a clockwise hexagon from its width across flats.
    auto hexagon = [](G4double width)
    {
        const G4double a = width/2;
        const G4double r = width/std::sqrt(3.0);

        return std::vector<G4TwoVector>{
            { r,   0},
            { r/2, -a},
            {-r/2, -a},
            {-r,   0},
            {-r/2,  a},
            { r/2,  a}
        };
    };

    auto holeSolid = new G4ExtrudedSolid(
        "Hole", hexagon(holeWidth), thickness/2
    );

    auto holeLogical = new G4LogicalVolume(
        holeSolid,
        worldLogical->GetMaterial(),
        "CollimatorHole"
    );

    // One reusable lead ring for displaying the channel walls.
    // Each ring supplies half the wall toward its neighbor.
    const G4double pitch = holeWidth + septum;

    auto outerSolid = new G4ExtrudedSolid(
        "CellOutside", hexagon(pitch), thickness/2
    );

    auto cutterSolid = new G4ExtrudedSolid(
        "CellCutter", hexagon(holeWidth), thickness/2 + 1*mm
    );

    auto ringSolid = new G4SubtractionSolid(
        "LeadRing", outerSolid, cutterSolid
    );

    auto ringLogical = new G4LogicalVolume(
        ringSolid, lead, "LeadRing"
    );

    const G4double pitchX = std::sqrt(3.0)*pitch/2;
    const G4double pitchY = pitch;

    const G4double holeRadius = holeWidth/std::sqrt(3.0);
    const G4double outerRadius = pitch/std::sqrt(3.0);

    const G4int columns =
        static_cast<G4int>(halfX/pitchX) + 1;
    const G4int rows =
        static_cast<G4int>(halfY/pitchY) + 1;

    G4int id = 0;

    for (G4int column = -columns; column <= columns; ++column)
    {
        const G4double x = column*pitchX;
        const G4double offset =
            (column % 2 != 0) ? pitchY/2 : 0;

        for (G4int row = -rows; row <= rows; ++row)
        {
            const G4double y = row*pitchY + offset;

            // Keep complete holes inside the plate.
            if (std::abs(x) + holeRadius > halfX - septum)
                continue;

            if (std::abs(y) + holeWidth/2 > halfY - septum)
                continue;

            new G4PVPlacement(
                nullptr,
                G4ThreeVector(x, y, 0),
                holeLogical,
                "CollimatorHole",
                blockLogical,
                false,
                id,
                false
            );

            // Draw a ring only where its whole outline fits.
            if (std::abs(x) + outerRadius < halfX &&
                std::abs(y) + pitch/2 < halfY)
            {
                new G4PVPlacement(
                    nullptr,
                    G4ThreeVector(x, y, 0),
                    ringLogical,
                    "LeadRing",
                    blockLogical,
                    false,
                    id,
                    false
                );
            }

            ++id;
        }
    }

    // Show the block outline; solid rings show the lead walls.
    auto outline = new G4VisAttributes(
        G4Colour(0.5, 0.5, 0.5)
    );
    outline->SetForceWireframe(true);
    blockLogical->SetVisAttributes(outline);

    auto walls = new G4VisAttributes(
        G4Colour(0.6, 0.6, 0.6, 1.0)
    );
    walls->SetForceSolid(true);
    ringLogical->SetVisAttributes(walls);

    holeLogical->SetVisAttributes(
        G4VisAttributes::GetInvisible()
    );
}
