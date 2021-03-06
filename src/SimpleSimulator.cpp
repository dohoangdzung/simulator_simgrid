/**
 * Copyright (c) 2020. <ADD YOUR HEADER INFORMATION>.
 * Generated with the wrench-init.in tool.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <wrench.h>
#include "SimpleStandardJobScheduler.h"
#include "SimpleWMS.h"
#include "PipelineWMS.h"

static bool ends_with(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

wrench::Workflow *workflow_exp1(char *workflow_file) {

    wrench::Workflow *workflow;
    if (ends_with(workflow_file, "dax")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromDAX(workflow_file, "1000Gf");
    } else if (ends_with(workflow_file, "json")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromJSON(workflow_file, "1000Gf");
    } else {
        std::cerr << "Workflow file name must end with '.dax' or '.json'" << std::endl;
        exit(1);
    }
    return workflow;
}

wrench::Workflow *workflow_exp2(int num_pipes, int num_tasks, int core_per_task,
                                long flops, long file_size, long mem_required) {

    wrench::Workflow *workflow = new wrench::Workflow();

    for (int i = 0; i < num_pipes; i++) {

        /* Add workflow tasks */
        for (int j = 0; j < num_tasks; j++) {
            /* Create a task: 10GFlop, single core */
            auto task = workflow->addTask("task_" + std::to_string(i) + "_" + std::to_string(j),
                                         flops, 1, core_per_task, 1, mem_required);
        }

        /* Add workflow files */
        for (int j = 0; j < num_tasks + 1; j++) {
            workflow->addFile("file_" + std::to_string(i) + "_" + std::to_string(j), file_size);
        }

        /* Create tasks and set input/output files for each task */
        for (int j = 0; j < num_tasks; j++) {

            auto task = workflow->getTaskByID("task_" + std::to_string(i) + "_" + std::to_string(j));

            task->addInputFile(workflow->getFileByID("file_" + std::to_string(i) + "_" + std::to_string(j)));
            task->addOutputFile(workflow->getFileByID("file_" + std::to_string(i) + "_" + std::to_string(j + 1)));
        }
    }

    return workflow;
}

void export_output_exp1(wrench::SimulationOutput output, int num_tasks, std::string filename){
    auto read_start = output.getTrace<wrench::SimulationTimestampFileReadStart>();
    auto read_end = output.getTrace<wrench::SimulationTimestampFileReadCompletion>();
    auto write_start = output.getTrace<wrench::SimulationTimestampFileWriteStart>();
    auto write_end = output.getTrace<wrench::SimulationTimestampFileWriteCompletion>();
    auto task_start = output.getTrace<wrench::SimulationTimestampTaskStart>();
    auto task_end = output.getTrace<wrench::SimulationTimestampTaskCompletion>();

    FILE *log_file = fopen(filename.c_str(), "w");
    fprintf(log_file, "type, start, end\n");

    for (int i = 0; i < num_tasks; i++) {
//        std::cerr << "Task " << read_end[i]->getContent()->getTask()->getID()
//                  << " read completed in " << read_end[i]->getDate() - read_start[i]->getDate()
//                  << std::endl;
//        std::cerr << "Task " << read_end[i]->getContent()->getTask()->getID()
//                  << " write completed in " << write_end[i]->getDate() - write_start[i]->getDate()
//                  << std::endl;
        std::cerr << "Task " << read_end[i]->getContent()->getTask()->getID()
                  << " completed at " << task_end[i]->getDate()
                  << " in " << task_end[i]->getDate() - task_start[i]->getDate()
                  << std::endl;


        fprintf(log_file, "read, %lf, %lf\n", read_start[i]->getDate(), read_end[i]->getDate());
        fprintf(log_file, "write, %lf, %lf\n", write_start[i]->getDate(), write_end[i]->getDate());
    }

    fclose(log_file);
}

void export_output_exp2(wrench::SimulationOutput output, int num_tasks, std::string filename){
    auto read_start = output.getTrace<wrench::SimulationTimestampFileReadStart>();
    auto read_end = output.getTrace<wrench::SimulationTimestampFileReadCompletion>();
    auto write_start = output.getTrace<wrench::SimulationTimestampFileWriteStart>();
    auto write_end = output.getTrace<wrench::SimulationTimestampFileWriteCompletion>();
    auto task_start = output.getTrace<wrench::SimulationTimestampTaskStart>();
    auto task_end = output.getTrace<wrench::SimulationTimestampTaskCompletion>();

    FILE *log_file = fopen(filename.c_str(), "w");
    fprintf(log_file, "read_start, read_end, cpu_start, cpu_end, write_start, write_end\n");

    for (int i = 0; i < num_tasks; i++) {
        std::cerr << read_end[i]->getContent()->getTask()->getID()
                  << " started at " << task_start[i]->getDate()
                  << ", ended at " << task_end[i]->getDate()
                  << ", completed in " << task_end[i]->getDate() - task_start[i]->getDate()
                  << std::endl;

        fprintf(log_file, "%lf, %lf, %lf, %lf, %lf, %lf\n", read_start[i]->getDate(), read_end[i]->getDate(),
                read_end[i]->getDate(), write_start[i]->getDate(),
                write_start[i]->getDate(), write_end[i]->getDate());
    }

    fclose(log_file);
}

int main(int argc, char **argv) {

    // Declaration of the top-level WRENCH simulation object
    wrench::Simulation simulation;

    // Initialization of the simulation
    simulation.init(&argc, argv);

    // Parsing of the command-line arguments for this WRENCH simulation
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file>" << std::endl;
        exit(1);
    }

    // The first argument is the platform description file, written in XML following the SimGrid-defined DTD
    char *platform_file = argv[1];
    // The second argument is the workflow description file, written in XML using the DAX DTD
    char *workflow_file = argv[2];

    int no_pipe = 1;

    // Reading and parsing the workflow description file to create a wrench::Workflow object
    std::cerr << "Loading workflow..." << std::endl;
    wrench::Workflow *workflow = workflow_exp1(workflow_file);
//    wrench::Workflow *workflow = workflow_exp2(1, 3, 1, 0, 3000000000, 3000000000);


    std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;

    // Reading and parsing the platform description file to instantiate a simulated platform
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation.instantiatePlatform(platform_file);

    // Get a vector of all the hosts in the simulated platform
    std::vector<std::string> hostname_list = simulation.getHostnameList();

    // Instantiate a storage service
//  std::string storage_host = hostname_list[(hostname_list.size() > 2) ? 2 : 1];
    std::string storage_host = hostname_list[0];
    std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
    auto storage_service = simulation.add(new wrench::SimpleStorageService(storage_host, {"/"}));

    // Construct a list of hosts (in this example only one host)
//  std::string executor_host = hostname_list[(hostname_list.size() > 1) ? 1 : 0];
    std::string executor_host = hostname_list[0];
    std::vector<std::string> execution_hosts = {executor_host};

    // Create a list of storage services that will be used by the WMS
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;
    storage_services.insert(storage_service);

    // Create a list of compute services that will be used by the WMS
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;

    std::string wms_host = hostname_list[0];
    // Instantiate a cloud service and add it to the simulation
    try {
        auto compute_service = new wrench::BareMetalComputeService(
                wms_host, execution_hosts, "", {}, {});

        compute_services.insert(simulation.add(compute_service));
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    // Instantiate a WMS
    auto wms = simulation.add(new wrench::PipelineWMS(compute_services, storage_services, wms_host));
    wms->addWorkflow(workflow);

    // Instantiate a file registry service
//  std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    std::string file_registry_service_host = hostname_list[0];
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
            new wrench::FileRegistryService(file_registry_service_host);
    simulation.add(file_registry_service);

    // It is necessary to store, or "stage", input files
    std::cerr << "Staging input files..." << std::endl;
    auto input_files = workflow->getInputFiles();
    try {
        for (auto const &f : input_files) {
            simulation.stageFile(f, storage_service);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }

    // Launch the simulation
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Simulation done!" << std::endl;

    export_output_exp1(simulation.getOutput(), workflow->getNumberOfTasks(),
            "timestamp_sim_exp1_75gb.csv");

    return 0;
}