#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4MaterialPropertiesTable.hh" //since we're adding #2 optical properties 
        //REMOVED WHEN I WAS SEPARATING THE SIPM ARRAY OUT   #include "G4OpticalSurface.hh"          //THIS AND #3
        //REMOVED, SAME REASON AS ABOVE                      #include "G4LogicalBorderSurface.hh"    //THIS for #3 SiPM addition
        //INSTEAD we have \/ 
#include "SiPMArray.hh"   //the separated part...

//SETTING THE WORLD 
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto material =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");

    auto solid = new G4Box("World", 100*cm, 100*cm, 100*cm); 
    //auto solid = new G4Box("World", 50*cm, 50*cm, 50*cm); 
    // Here is where the dim of the world is defined /\ 
    // so '50*cm, 50*cm, 50*cm' means the world is of dim 100*100*100 cm

    auto logical =
        new G4LogicalVolume(solid, material, "World");

    auto physical = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logical,
        "World",
        nullptr,
        false,
        0,
        true
    );
     // BUILDING THE CRYSTAL/SCINTILLATOR ITSELF DOWN HERE \/

     // CHoosing the Material: cesium iodide (CsI)
    auto crystalMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial(
            "G4_CESIUM_IODIDE"
        );



//============================================================================================================================
    // Teaching values: replace with documented crystal properties later.                                                    
    // All arrays have 3 entries, matched by position.                                                                       
    G4double photonEnergy[]     = {2.0*eV, 2.25*eV, 2.5*eV};
    G4double emission[]         = {0.2, 1.0, 0.2};                               // Relative intensity at each energy
    G4double crystalIndex[]     = {1.79, 1.79, 1.79};                            // Refractive index at each energy
    G4double absorptionLength[] = {1*m, 1*m, 1*m};                               // Mean distance before absorption

    auto crystalProperties = new G4MaterialPropertiesTable();

    // Energy-dependent properties; 3 = number of array entries
    crystalProperties->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, emission, 3);
    crystalProperties->AddProperty("RINDEX", photonEnergy, crystalIndex, 3);
    crystalProperties->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, 3);

    // Properties independent of photon energy
    crystalProperties->AddConstProperty("SCINTILLATIONYIELD", 54000.0/MeV);      // Mean photons per MeV deposited
    crystalProperties->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1000*ns);  // Emission decay time
    crystalProperties->AddConstProperty("SCINTILLATIONYIELD1", 1.0);             // Fraction in component 1: all light
    crystalProperties->AddConstProperty("RESOLUTIONSCALE", 1.0);                 // Photon-count fluctuation scale

    crystalMaterial->SetMaterialPropertiesTable(crystalProperties);

    // Near-vacuum world: refractive index = 1 at all three energies
    G4double worldIndex[] = {1.0, 1.0, 1.0};
    auto worldProperties = new G4MaterialPropertiesTable();
    worldProperties->AddProperty("RINDEX", photonEnergy, worldIndex, 3);
    material->SetMaterialPropertiesTable(worldProperties);

//============================================================================================================================



    // Defining its Shape: a 2 cm cube (G4Box takes half-lengths)
    auto crystalSolid =
        new G4Box("Crystal", 8*cm, 8*cm, 4*cm);
        //so each dimension given here (say 1cm) extends it to that amount
        //so if you need a box of dimensions 2*2*2 then you give 1*cm, 1*cm, 1*cm 

    // Shape combined with material
    auto crystalLogical =
        new G4LogicalVolume(
            crystalSolid,
            crystalMaterial,
            "Crystal"
        );

    // Placing the crystal in the world- (0,0,0) to place it at origin
    new G4PVPlacement(
        nullptr,                // no rotation
        G4ThreeVector(0, 0, 0), //position its placed at
        crystalLogical,
        "Crystal",              //name of the placement?
        logical,
        false,
        0,                      // sort of like the ID, imp if you have multiple of these
        true                    // checking for overlaps? confirm later
    );

    // COlor of the item
    auto crystalAppearance =
        new G4VisAttributes(G4Colour(0.0, 1.0, 1.0)); //color, RGB format

    crystalAppearance->SetForceWireframe(true);
    crystalLogical->SetVisAttributes(crystalAppearance);
    
    BuildSiPMArray(logical, crystalSolid, photonEnergy);

    return physical;
}