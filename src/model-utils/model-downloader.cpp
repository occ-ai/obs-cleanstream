#include "model-downloader.h"
#include "plugin-support.h"
#include "model-downloader-ui.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <curl/curl.h>

bool check_if_model_exists(const std::string &model_name)
{
	obs_log(LOG_INFO, "Checking if model %s exists...", model_name.c_str());
	char *model_file_path = obs_module_file(model_name.c_str());
	obs_log(LOG_INFO, "Model file path: %s", model_file_path);
	if (model_file_path == nullptr) {
		obs_log(LOG_INFO, "Model %s does not exist.", model_name.c_str());
		return false;
	}

	if (!std::filesystem::exists(model_file_path)) {
		obs_log(LOG_INFO, "Model %s does not exist.", model_file_path);
		bfree(model_file_path);
		return false;
	}
	bfree(model_file_path);
	return true;
}

std::string find_file_in_folder_by_name(const std::string &folder_path,
					const std::string &file_name)
{
	for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
		if (entry.path().filename() == file_name) {
			return entry.path().string();
		}
	}
	return "";
}

std::string find_bin_file_in_folder(const std::string &model_local_folder_path)
{
	// find .bin file in folder
	for (const auto &entry : std::filesystem::directory_iterator(model_local_folder_path)) {
		if (entry.path().extension() == ".bin") {
			const std::string bin_file_path = entry.path().string();
			obs_log(LOG_INFO, "Model bin file found in folder: %s",
				bin_file_path.c_str());
			return bin_file_path;
		}
	}
	obs_log(LOG_ERROR, "Model bin file not found in folder: %s",
		model_local_folder_path.c_str());
	return "";
}

std::string find_model_folder(const ModelInfo &model_info)
{
	char *data_folder_models = obs_module_file("models");
	const std::filesystem::path module_data_models_folder =
		std::filesystem::absolute(data_folder_models);
	bfree(data_folder_models);

	const std::string model_local_data_path =
		(module_data_models_folder / model_info.local_folder_name).string();

	obs_log(LOG_INFO, "Checking if model '%s' exists in data...",
		model_info.friendly_name.c_str());

	if (!std::filesystem::exists(model_local_data_path)) {
		obs_log(LOG_INFO, "Model not found in data: %s", model_local_data_path.c_str());
	} else {
		obs_log(LOG_INFO, "Model folder found in data: %s", model_local_data_path.c_str());
		return model_local_data_path;
	}

	// Check if model exists in the config folder
	char *config_folder = obs_module_get_config_path(obs_current_module(), "models");
	const std::filesystem::path module_config_models_folder =
		std::filesystem::absolute(config_folder);
	bfree(config_folder);

	obs_log(LOG_INFO, "Checking if model '%s' exists in config...",
		model_info.friendly_name.c_str());

	const std::string model_local_config_path =
		(module_config_models_folder / model_info.local_folder_name).string();

	obs_log(LOG_INFO, "Model path in config: %s", model_local_config_path.c_str());
	if (std::filesystem::exists(model_local_config_path)) {
		obs_log(LOG_INFO, "Model exists in config folder: %s",
			model_local_config_path.c_str());
		return model_local_config_path;
	}

	obs_log(LOG_INFO, "Model '%s' not found.", model_info.friendly_name.c_str());
	return "";
}

std::string find_model_bin_file(const ModelInfo &model_info)
{
	const std::string model_local_folder_path = find_model_folder(model_info);
	if (model_local_folder_path.empty()) {
		return "";
	}

	return find_bin_file_in_folder(model_local_folder_path);
}

void download_model_with_ui_dialog(const ModelInfo &model_info,
				   download_finished_callback_t download_finished_callback)
{
	// Start the model downloader UI
	ModelDownloader *model_downloader = new ModelDownloader(
		model_info, download_finished_callback, (QWidget *)obs_frontend_get_main_window());
	model_downloader->show();
}
