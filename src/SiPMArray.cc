#include "SiPMArray.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"

void BuildSiPMArray(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid,
    G4double* photonEnergy
)
//UNDERSTAND THE CODE BELOW PART BY PART
{
    
    auto logical = worldLogical;
    
    //========================================================================================================================

    const G4int rows = 4;
    const G4int columns = 4;

    // Effective active area of each readout channel, not a commercial die.
    const G4double sensorWidth = 38*mm;
    const G4double sensorHeight = 38*mm;

    // Silicon thickness; packaging and microcell structure are omitted.
    const G4double sensorThickness = 0.5*mm;

    // Centre-to-centre separation; gives 2 mm gaps between active areas.
    const G4double pitchX = 40*mm;
    const G4double pitchY = 40*mm;

    // Effective optical coupling layer thickness.
    const G4double couplingThickness = 0.5*mm;

    // Move the coupling layer and array together in x/y.
    // Keep zero initially so they cover the crystal rear face.
    const G4double arrayOffsetX = 0*mm;
    const G4double arrayOffsetY = 0*mm;

    // Crystal is currently centred at z=0.
    // Change this if you later move the crystal.
    const G4double crystalCentreZ = 0*mm;
    const G4double crystalRearZ =
        crystalCentreZ + crystalSolid->GetZHalfLength();

    // Coupling touches the crystal; sensors touch the coupling layer.
    const G4double couplingCentreZ =
        crystalRearZ + couplingThickness/2;

    const G4double sensorCentreZ =
        crystalRearZ + couplingThickness + sensorThickness/2;

    // ===== EFFECTIVE OPTICAL COUPLING MATERIAL =====

    // Polymer approximation for optical coupling.
    // Replace its composition/properties later if a real grease is selected.
    auto couplingMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_PLEXIGLASS");

    // Assumed constant index over the existing three photon energies.
    G4double couplingIndex[] = {1.50, 1.50, 1.50};

    // Assumed bulk attenuation length; not measured coupling data.
    G4double couplingAbsorption[] = {1*m, 1*m, 1*m};

    auto couplingProperties = new G4MaterialPropertiesTable();
    couplingProperties->AddProperty(
        "RINDEX", photonEnergy, couplingIndex, 3);
    couplingProperties->AddProperty(
        "ABSLENGTH", photonEnergy, couplingAbsorption, 3);

    couplingMaterial->SetMaterialPropertiesTable(couplingProperties);

    // Full-face coupling sheet; dimensions follow the crystal.
    auto couplingSolid = new G4Box(
        "OpticalCoupling",
        crystalSolid->GetXHalfLength(),
        crystalSolid->GetYHalfLength(),
        couplingThickness/2
    );

    auto couplingLogical = new G4LogicalVolume(
        couplingSolid, couplingMaterial, "OpticalCoupling");

    auto couplingPhysical = new G4PVPlacement(
        nullptr,
        G4ThreeVector(arrayOffsetX, arrayOffsetY, couplingCentreZ),
        couplingLogical,
        "OpticalCoupling",
        logical,
        false,
        0,
        true
    );

    auto couplingAppearance =
        new G4VisAttributes(G4Colour(1.0, 1.0, 0.0));
    couplingAppearance->SetForceWireframe(true);
    couplingLogical->SetVisAttributes(couplingAppearance);

    // ===== SENSOR MATERIAL AND ACTIVE SURFACE =====

    auto sensorMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Si");

    auto sensorSolid = new G4Box(
        "SiPM",
        sensorWidth/2,
        sensorHeight/2,
        sensorThickness/2
    );

    auto sensorLogical =
        new G4LogicalVolume(sensorSolid, sensorMaterial, "SiPM");

    auto sensorAppearance =
        new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
    sensorAppearance->SetForceSolid(true);
    sensorLogical->SetVisAttributes(sensorAppearance);

    // Assumed effective probability of detection per incident photon.
    // Includes unresolved microcell fill factor and avalanche probability.
    // Constant with wavelength and angle for this simplified model.
    const G4double assumedPDE = 0.30;

    // Assumed probability of optical reflection at the active surface.
    // Keep below 1, and require assumedPDE <= 1 - sensorReflectivity.
    const G4double sensorReflectivity = 0.10;

    // Geant4 EFFICIENCY is conditional on absorption.
    // This gives an overall incident detection probability of 0.30:
    // (1 - 0.10) * (0.30 / 0.90) = 0.30.
    const G4double conditionalEfficiency =
        assumedPDE / (1.0 - sensorReflectivity);

    G4double reflection[] = {
        sensorReflectivity, sensorReflectivity, sensorReflectivity
    };

    G4double efficiency[] = {
        conditionalEfficiency,
        conditionalEfficiency,
        conditionalEfficiency
    };

    auto sensorSurface = new G4OpticalSurface("SiPMActiveSurface");

    // Effective absorbing/reflecting photodetector boundary.
    // This is a surface-response model, not a microscopic silicon model.
    sensorSurface->SetType(dielectric_metal);
    sensorSurface->SetModel(unified);
    sensorSurface->SetFinish(polished);

    auto sensorProperties = new G4MaterialPropertiesTable();
    sensorProperties->AddProperty(
        "REFLECTIVITY", photonEnergy, reflection, 3);
    sensorProperties->AddProperty(
        "EFFICIENCY", photonEnergy, efficiency, 3);
    sensorSurface->SetMaterialPropertiesTable(sensorProperties);

    // ===== PLACE SIXTEEN CHANNELS =====

    for (G4int row = 0; row < rows; ++row)
    {
        for (G4int column = 0; column < columns; ++column)
        {
            const G4int sensorID = row*columns + column;

            const G4double x =
                arrayOffsetX + (column - (columns-1)/2.0)*pitchX;

            const G4double y =
                arrayOffsetY + (row - (rows-1)/2.0)*pitchY;

            auto sensorPhysical = new G4PVPlacement(
                nullptr,
                G4ThreeVector(x, y, sensorCentreZ),
                sensorLogical,
                "SiPM",
                logical,
                false,
                sensorID,  // Unique channel number: 0 to 15
                true       // Check geometry overlaps
            );

            // Detection response applies only when light travels
            // from the coupling layer into this sensor's front face.
            new G4LogicalBorderSurface(
                "CouplingToSiPM",
                couplingPhysical,
                sensorPhysical,
                sensorSurface
            );
        }
    }


//========================================================================================================================
    
}