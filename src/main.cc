#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4RunManager.hh"
#include "FTFP_BERT.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4OpticalPhysics.hh"  //for #optical physics... since we are adding photons during scintillation

#include "EventAction.hh"       //for stepping action.. 'pre and post step'
#include "SteppingAction.hh"    //for stepping action.. 'pre and post step'

int main(int argc, char** argv)
{
    auto runManager = new G4RunManager();

    runManager->SetUserInitialization(new DetectorConstruction());
    // runManager->SetUserInitialization(new FTFP_BERT());
    //removed above line whilst adding #optical physics, replaced with below line
    auto physicsList = new FTFP_BERT();
    physicsList->RegisterPhysics(new G4OpticalPhysics());
    runManager->SetUserInitialization(physicsList);
    runManager->SetUserAction(new PrimaryGeneratorAction());
    
//________________________________________________________________
//_________________________STEPPING ACTION________________________|
    auto eventAction = new EventAction();                       //|
                                                                //|
    runManager->SetUserAction(eventAction);                     //|
    runManager->SetUserAction(new SteppingAction(eventAction)); //|
//________________________________________________________________|

    auto visManager = new G4VisExecutive();
    visManager->Initialize();

    auto commands = G4UImanager::GetUIpointer();

    if (argc > 1)
    {
        commands->ApplyCommand(
            G4String("/control/execute ") + argv[1]
        );
    }
    else
    {
        auto ui = new G4UIExecutive(argc, argv);
        commands->ApplyCommand("/control/execute macros/vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;

    return 0;
}