#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4RunManager.hh"
#include "FTFP_BERT.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

int main(int argc, char** argv)
{
    auto runManager = new G4RunManager();

    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new FTFP_BERT());
    runManager->SetUserAction(new PrimaryGeneratorAction());

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