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
{
    
    auto logical = worldLogical; //named


    //NOTE: const here means the value cannot be reassigned after the initialization
    //=============================================================================================================================================|
    //                                                                                                                                             |
    const G4int rows = 4;                                                                //NUMBER OF SENSORS-    ROWS                              |
    const G4int columns = 4;                                                             //NUMBER OF SENSORS-   COLUMNS                            |
    const G4double sensorWidth = 40*mm;                                                  //SENSOR WIDTH            X                               |
    const G4double sensorHeight = 40*mm;                                                 //SENSOR HEIGHT           Y                               |
    const G4double sensorThickness = 4*mm;                                             //SENSOR THICKNESS        Z                               |
    // Centre-to-centre separation                                                                                                                 |
    const G4double pitchX = 40*mm;                                                       //CENTRE TO CENTRE DISTANCE X                             |
    const G4double pitchY = 40*mm;                                                       //CENTRE TO CENTRE DISTANCE Y                             |
    //                                                                                                                                             |                            |        
    const G4double arrayOffsetX = 0*mm;                                                  //X OFFSET                                                |
    const G4double arrayOffsetY = 0*mm;                                                  //Y OFFSET                                                |
    //                                                                                                                                             |
    // Crystal is currently centred at z=0.                                                                                                        |
    // Change this if you later move the crystal.                                                                                                  |
    const G4double crystalCentreZ = 0*mm;                                                //CRYSTAL'S POSITION (IF CHANGED, EDIT THIS ASWELL)       |
    const G4double crystalRearZ =                                                                                                                //|
        crystalCentreZ + crystalSolid->GetZHalfLength();                                                                                         //|
                                                                                                                                                 //|
    //ensures SENSOR TOUCHES CRYSTAL:                                                                                                            //|
    const G4double sensorCentreZ =                                                                                                               //|
        crystalRearZ + sensorThickness/2;                                                                                                        //|
                                                                                                                                                 //|
    //=============================================================================================================================================|

    

//________________________________________________ DELETED    ||  OPTICAL  COUPLING  ||    DELETED________________________________________________
    // // ================================================================================================= //    
    // // =========================== EFFECTIVE OPTICAL COUPLING MATERIAL ================================= //
    // // ================================ DELETED OPTICAL COUPLING ======================================= //
    // // ================================================================================================= //

    // auto couplingMaterial =
    //     G4NistManager::Instance()->FindOrBuildMaterial("G4_PLEXIGLASS"); //MATERIAL FOR OPTICAL COUPLING

    // // Assumed data, change for accurate values later:
    // G4double couplingIndex[] = {1.50, 1.50, 1.50};                       //HOW COUPLING MATERIAL AFFECTS PHOTONS
    // G4double couplingAbsorption[] = {1*m, 1*m, 1*m};                     // same...

    // //______________________________________________________________________________________________________________________
    // //PUT PROPERTIES INTO TABLE                                            !!! PUT PROPERTIES INTO TABLE !!!              //|             
    // auto couplingProperties = new G4MaterialPropertiesTable();                                                            //|
    // couplingProperties->AddProperty(                                                                                      //|
    //     "RINDEX", photonEnergy, couplingIndex, 3);                                                                        //|
    // couplingProperties->AddProperty(                                                                                      //|
    //     "ABSLENGTH", photonEnergy, couplingAbsorption, 3);                                                                //|
    //                                                                                                                       //|
    // couplingMaterial->SetMaterialPropertiesTable(couplingProperties); //ATTACH table data/parameters to the material itself |
    // //______________________________________________________________________________________________________________________|

    // // MAKING IT SO THAT the coupling sheet's dimensions follow the crystal.
    // auto couplingSolid = new G4Box(
    //     "OpticalCoupling",
    //     crystalSolid->GetXHalfLength(),
    //     crystalSolid->GetYHalfLength(),
    //     couplingThickness/2
    // );

    // auto couplingLogical = new G4LogicalVolume(
    //     couplingSolid, couplingMaterial, "OpticalCoupling");  //COMBINE SHAPE + MATERIAL

    // auto couplingPhysical = new G4PVPlacement(                //NOW GIVING IT A POSITION
    //     nullptr,                                                      //no rotation
    //     G4ThreeVector(arrayOffsetX, arrayOffsetY, couplingCentreZ),   //Position of centre
    //     couplingLogical,                                              //shape+material definition to place
    //     "OpticalCoupling",                                            //name of THIS placement
    //     logical,                                                      //volume (ie this world)
    //     false,                                                        
    //     0,                                                            //ID of THIS placement
    //     true                                                          //CHecking overlaps
    // );

    // //APPEARANCE
    // auto couplingAppearance =
    //     new G4VisAttributes(G4Colour(1.0, 1.0, 0.0));    //RGB VAlues for color
    // couplingAppearance->SetForceWireframe(true);
    // couplingLogical->SetVisAttributes(couplingAppearance);


    // ===================================== SENSOR MATERIAL AND ACTIVE SURFACE =====================================

    auto sensorMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Si");   //SELECTED MATERIAL FOR SENSOR (SILICON)

// // =================================STEPPING ACTION ADDITION============================================|
//                                                                                                         |
    // Ideal collector: match the crystal's refractive index.                                              |
    // Keep these values matched if the crystal index changes.                                             |
    G4double sensorIndex[] = {1.79, 1.79, 1.79};                                                         //|
//                                                                                                         |
    auto sensorProperties = new G4MaterialPropertiesTable();                                             //|
//                                                                                                         |
    sensorProperties->AddProperty(                                                                       //|
        "RINDEX", photonEnergy, sensorIndex, 3);                                                         //|
//                                                                                                         |
    sensorMaterial->SetMaterialPropertiesTable(sensorProperties);                                        //|
//                                                                                                         |
// // =====================================================================================================|




    auto sensorSolid = new G4Box(             //sensor shape and logical volume
        "SiPM", 
        sensorWidth/2,
        sensorHeight/2,
        sensorThickness/2
    );

    auto sensorLogical =
        new G4LogicalVolume(sensorSolid, sensorMaterial, "SiPM"); //combines shape and material

    auto sensorAppearance =                                 //APPEARANCE
        new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));       //RGB
    sensorAppearance->SetForceSolid(true);
    sensorLogical->SetVisAttributes(sensorAppearance);

    // =================================================================================================================================================================================

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
    }
}
}