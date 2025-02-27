/*
 * Wazuh Module for Agent Upgrading
 * Copyright (C) 2015, Wazuh Inc.
 * July 20, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wazuh_db/helpers/wdb_global_helpers.h"
#include "wazuh_modules/wmodules.h"
#include "wm_agent_upgrade_validate.h"

#ifdef WAZUH_UNIT_TESTING
// Redefine ossec_version
#undef __ossec_version
#define __ossec_version "v3.13.0"
#endif

// Mutex needed to download a WPK file
pthread_mutex_t download_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char* invalid_platforms[] = {
    "sunos",
    "aix",
    "hp-ux",
    "bsd"
};

static const char* rolling_platforms[] = {
    "opensuse-tumbleweed",
    "arch"
};

int wm_agent_upgrade_validate_id(int agent_id) {
    int return_code = WM_UPGRADE_SUCCESS;

    if (agent_id == MANAGER_ID) {
        return_code = WM_UPGRADE_INVALID_ACTION_FOR_MANAGER;
    }

    return return_code;
}

int wm_agent_upgrade_validate_status(const char* connection_status) {
    int return_code = WM_UPGRADE_AGENT_IS_NOT_ACTIVE;

    if (connection_status && !strcmp(AGENT_CS_ACTIVE, connection_status)) {
        return_code = WM_UPGRADE_SUCCESS;
    }

    return return_code;
}

int wm_agent_upgrade_validate_system(const char *platform, const char *os_major, const char *os_minor, const char *arch) {
    int invalid_platforms_len = 0;
    int rolling_platforms_len = 0;
    int platforms_it = 0;
    int return_code = WM_UPGRADE_GLOBAL_DB_FAILURE;

    if (platform) {
        if (!strcmp(platform, "windows") || (os_major && arch && (strcmp(platform, "ubuntu") || os_minor))) {
            return_code = WM_UPGRADE_SUCCESS;

            // Blacklist for invalid OS platforms
            invalid_platforms_len = array_size(invalid_platforms);
            for(platforms_it = 0; platforms_it < invalid_platforms_len; ++platforms_it) {
                if(!strcmp(invalid_platforms[platforms_it], platform)) {
                    return_code = WM_UPGRADE_SYSTEM_NOT_SUPPORTED;
                    break;
                }
            }

            if (WM_UPGRADE_SUCCESS == return_code) {
                if ((!strcmp(platform, "sles") && !strcmp(os_major, "11")) ||
                    (!strcmp(platform, "rhel") && !strcmp(os_major, "5")) ||
                    (!strcmp(platform, "centos") && !strcmp(os_major, "5"))) {
                    return_code = WM_UPGRADE_SYSTEM_NOT_SUPPORTED;
                }
            }
        }

        // Whitelist for OS platforms with 'rolling' version
        rolling_platforms_len = array_size(rolling_platforms);
        for(platforms_it = 0; platforms_it < rolling_platforms_len; ++platforms_it) {
            if(!strcmp(rolling_platforms[platforms_it], platform)) {
                return_code = WM_UPGRADE_SUCCESS;
                break;
            }
        }
    }

    return return_code;
}

int wm_agent_upgrade_validate_version(const char *wazuh_version, const char *platform, wm_upgrade_command command, void *task) {
    char *tmp_agent_version = NULL;
    char *manager_version = NULL;
    int return_code = WM_UPGRADE_GLOBAL_DB_FAILURE;

    if (wazuh_version) {
        if (tmp_agent_version = strchr(wazuh_version, 'v'), tmp_agent_version) {

            if (wm_agent_upgrade_compare_versions(tmp_agent_version, WM_UPGRADE_MINIMAL_VERSION_SUPPORT) < 0) {
                return_code = WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED;
            } else if (wm_agent_upgrade_compare_versions(tmp_agent_version, WM_UPGRADE_MINIMAL_VERSION_SUPPORT_MACOS) < 0 && !strcmp(platform, "darwin")) {
                return_code = WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED;
            } else if (WM_UPGRADE_UPGRADE == command) {
                wm_upgrade_task *upgrade_task = (wm_upgrade_task *)task;

                if (manager_version = strchr(__ossec_version, 'v'), manager_version) {
                    return_code = WM_UPGRADE_SUCCESS;

                    os_strdup(upgrade_task->custom_version ? upgrade_task->custom_version : manager_version, upgrade_task->wpk_version);

                    if (!upgrade_task->force_upgrade) {
                        if (wm_agent_upgrade_compare_versions(tmp_agent_version, upgrade_task->wpk_version) >= 0) {
                            return_code = WM_UPGRADE_NEW_VERSION_LEES_OR_EQUAL_THAT_CURRENT;
                        } else if (wm_agent_upgrade_compare_versions(upgrade_task->wpk_version, manager_version) > 0) {
                            return_code = WM_UPGRADE_NEW_VERSION_GREATER_MASTER;
                        }
                    }
                }
            } else if (WM_UPGRADE_UPGRADE_CUSTOM == command) {
                return_code = WM_UPGRADE_SUCCESS;
            }
        }
    }

    return return_code;
}

int wm_agent_upgrade_validate_wpk_version(const wm_agent_info *agent_info, wm_upgrade_task *task, const char *wpk_repository_config) {

    char repository[OS_BUFFER_SIZE] = "";
    const char *http_tag = "http://";
    const char *https_tag = "https://";
    char *repository_url = NULL;
    char *path_url = NULL;
    char *file_url = NULL;
    char *versions_url = NULL;
    char *versions = NULL;
    int return_code = WM_UPGRADE_SUCCESS;
    int ver = 0;

    if (!task->wpk_version) {
        return WM_UPGRADE_WPK_VERSION_DOES_NOT_EXIST;
    }

    if (!task->wpk_repository) {
        if (wpk_repository_config) {
            os_strdup(wpk_repository_config, task->wpk_repository);
        } else if (wm_agent_upgrade_compare_versions(task->wpk_version, "v4.0.0") < 0) {
            os_strdup(WM_UPGRADE_WPK_REPO_URL_3_X, task->wpk_repository);
        } else {
            if (sscanf(task->wpk_version, "v%d.%*d.%*d", &ver) != 1 &&
                    sscanf(task->wpk_version, "%d.%*d.%*d", &ver) != 1) {
                return WM_UPGRADE_WPK_VERSION_DOES_NOT_EXIST;
            }
            snprintf(repository, OS_BUFFER_SIZE-1, WM_UPGRADE_WPK_REPO_URL, ver);
            os_strdup(repository, task->wpk_repository);
        }
    }

    os_calloc(OS_SIZE_1024, sizeof(char), repository_url);
    os_calloc(OS_SIZE_2048, sizeof(char), path_url);
    os_calloc(OS_SIZE_2048, sizeof(char), file_url);
    os_calloc(OS_SIZE_4096, sizeof(char), versions_url);

    // Set protocol
    if (!strstr(task->wpk_repository, http_tag) && !strstr(task->wpk_repository, https_tag)) {
        if (task->use_http) {
            strcat(repository_url, http_tag);
        } else {
            strcat(repository_url, https_tag);
        }
    }

    // Set repository
    strncat(repository_url, task->wpk_repository, OS_SIZE_512);
    if (task->wpk_repository[strlen(task->wpk_repository) - 1] != '/') {
        strcat(repository_url, "/");
    }

    // Set URL path
    if (!strcmp(agent_info->platform, "windows")) {
        snprintf(path_url, OS_SIZE_2048, "%swindows/",
                 repository_url);
        snprintf(file_url, OS_SIZE_2048, "wazuh_agent_%s_windows.wpk",
                 task->wpk_version);
    } else if (!strcmp(agent_info->platform, "darwin")) {
        snprintf(path_url, OS_SIZE_2048, "%smacos/%s/pkg/",
                 repository_url, agent_info->architecture);
        snprintf(file_url, OS_SIZE_2048, "wazuh_agent_%s_macos_%s.wpk",
                 task->wpk_version, agent_info->architecture);
    } else {
        if (wm_agent_upgrade_compare_versions(task->wpk_version, WM_UPGRADE_NEW_VERSION_REPOSITORY) >= 0) {
            snprintf(path_url, OS_SIZE_2048, "%slinux/%s/",
                     repository_url, agent_info->architecture);
            snprintf(file_url, OS_SIZE_2048, "wazuh_agent_%s_linux_%s.wpk",
                     task->wpk_version, agent_info->architecture);
        } else if (!strcmp(agent_info->platform, "ubuntu")) {
            snprintf(path_url, OS_SIZE_2048, "%s%s/%s.%s/%s/",
                     repository_url, agent_info->platform, agent_info->major_version, agent_info->minor_version, agent_info->architecture);
            snprintf(file_url, OS_SIZE_2048, "wazuh_agent_%s_%s_%s.%s_%s.wpk",
                     task->wpk_version, agent_info->platform, agent_info->major_version, agent_info->minor_version, agent_info->architecture);
        } else {
            snprintf(path_url, OS_SIZE_2048, "%s%s/%s/%s/",
                     repository_url, agent_info->platform, agent_info->major_version, agent_info->architecture);
            snprintf(file_url, OS_SIZE_2048, "wazuh_agent_%s_%s_%s_%s.wpk",
                     task->wpk_version, agent_info->platform, agent_info->major_version, agent_info->architecture);
        }
    }

    // Set versions respository
    snprintf(versions_url, OS_SIZE_4096, "%sversions", path_url);

    versions = wurl_http_get(versions_url, WM_UPGRADE_MAX_RESPONSE_SIZE);

    if (versions) {
        char *version = versions;
        char *sha1 = NULL;
        char *next_line = NULL;

        while (version) {
            if (next_line = strchr(version, '\n'), next_line) {
                *next_line = '\0';
                if (sha1 = strchr(version, ' '), sha1) {
                    *sha1 = '\0';
                    if (wm_agent_upgrade_compare_versions(task->wpk_version, version) == 0) {
                        // Save WPK url, file name and sha1
                        os_strdup(sha1 + 1, task->wpk_sha1);
                        os_strdup(file_url, task->wpk_file);
                        os_free(task->wpk_repository);
                        os_strdup(path_url, task->wpk_repository);
                        break;
                    }
                }
                version = next_line + 1;
            } else {
                break;
            }
        }
        if (version) {
            if (sha1 = strchr(version, ' '), sha1) {
                *sha1 = '\0';
                if (wm_agent_upgrade_compare_versions(task->wpk_version, version) == 0) {
                    // Save WPK url, file name and sha1
                    os_strdup(sha1 + 1, task->wpk_sha1);
                    os_strdup(file_url, task->wpk_file);
                    os_free(task->wpk_repository);
                    os_strdup(path_url, task->wpk_repository);
                }
            }
        }
        if (!task->wpk_repository || !task->wpk_file || !task->wpk_sha1) {
            return_code = WM_UPGRADE_WPK_VERSION_DOES_NOT_EXIST;
        }
    } else {
        return_code = WM_UPGRADE_URL_NOT_FOUND;
    }

    os_free(repository_url);
    os_free(path_url);
    os_free(file_url);
    os_free(versions_url);
    os_free(versions);

    return return_code;
}

int wm_agent_upgrade_validate_wpk(const wm_upgrade_task *task) {
    int return_code = WM_UPGRADE_SUCCESS;
    FILE *wpk_file = NULL;
    int exist = 0;
    int attempts = 0;
    int req = 0;
    char *file_url = NULL;
    char *file_path = NULL;
    os_sha1 file_sha1;

    if (task->wpk_repository && task->wpk_file && task->wpk_sha1) {

        // Take mutex to avoid downloading many times the same WPK
        w_mutex_lock(&download_mutex);

        os_calloc(OS_SIZE_4096, sizeof(char), file_url);
        os_calloc(OS_SIZE_4096, sizeof(char), file_path);

        snprintf(file_url, OS_SIZE_4096, "%s%s", task->wpk_repository, task->wpk_file);
        snprintf(file_path, OS_SIZE_4096, "%s%s", WM_UPGRADE_WPK_DEFAULT_PATH, task->wpk_file);

        if (wpk_file = fopen(file_path, "rb"), wpk_file) {
            if (!OS_SHA1_File(file_path, file_sha1, OS_BINARY) && !strcasecmp(file_sha1, task->wpk_sha1)) {
                // WPK already downloaded
                exist = 1;
            }
            fclose(wpk_file);
        }

        if (!exist) {
            mtdebug1(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_DOWNLOADING_WPK, file_url);

            // Download WPK file
            while (attempts++ < WM_UPGRADE_WPK_DOWNLOAD_ATTEMPTS) {
                if (req = wurl_request(file_url, file_path, NULL, NULL, WM_UPGRADE_WPK_DOWNLOAD_TIMEOUT), !req) {
                    if (OS_SHA1_File(file_path, file_sha1, OS_BINARY) || strcasecmp(file_sha1, task->wpk_sha1)) {
                        return_code = WM_UPGRADE_WPK_SHA1_DOES_NOT_MATCH;
                    }
                    break;
                } else if (attempts == WM_UPGRADE_WPK_DOWNLOAD_ATTEMPTS) {
                    return_code = WM_UPGRADE_WPK_FILE_DOES_NOT_EXIST;
                    break;
                }
                sleep(attempts);
            }
        }

        os_free(file_url);
        os_free(file_path);

        // Release download mutex
        w_mutex_unlock(&download_mutex);

    } else {
        return_code = WM_UPGRADE_WPK_FILE_DOES_NOT_EXIST;
    }

    return return_code;
}

int wm_agent_upgrade_validate_wpk_custom(const wm_upgrade_custom_task *task) {
    int return_code = WM_UPGRADE_SUCCESS;
    FILE *wpk_file = NULL;

    if (task->custom_file_path) {
        if (wpk_file = fopen(task->custom_file_path, "rb"), !wpk_file) {
            return_code = WM_UPGRADE_WPK_FILE_DOES_NOT_EXIST;
        } else {
            // WPK file exists
            fclose(wpk_file);
        }
    } else {
        return_code = WM_UPGRADE_WPK_FILE_DOES_NOT_EXIST;
    }

    return return_code;
}

int wm_agent_upgrade_compare_versions(const char *version1, const char *version2) {
    char ver1[10];
    char ver2[10];
    char *tmp_v1 = NULL;
    char *tmp_v2 = NULL;
    char *token = NULL;
    int patch1 = 0;
    int major1 = 0;
    int minor1 = 0;
    int patch2 = 0;
    int major2 = 0;
    int minor2 = 0;
    int result = 0;

    if (version1) {
        strncpy(ver1, version1, 9);

        if (tmp_v1 = strchr(ver1, 'v'), tmp_v1) {
            tmp_v1++;
        } else {
            tmp_v1 = ver1;
        }

        if (token = strtok(tmp_v1, "."), token) {
            major1 = atoi(token);

            if (token = strtok(NULL, "."), token) {
                minor1 = atoi(token);

                if (token = strtok(NULL, "."), token) {
                    patch1 = atoi(token);
                }
            }
        }
    }

    if (version2) {
        strncpy(ver2, version2, 9);

        if (tmp_v2 = strchr(ver2, 'v'), tmp_v2) {
            tmp_v2++;
        } else {
            tmp_v2 = ver2;
        }

        if (token = strtok(tmp_v2, "."), token) {
            major2 = atoi(token);

            if (token = strtok(NULL, "."), token) {
                minor2 = atoi(token);

                if (token = strtok(NULL, "."), token) {
                    patch2 = atoi(token);
                }
            }
        }
    }

    if (major1 > major2) {
        result = 1;
    } else if (major1 < major2){
        result = -1;
    } else {
        if(minor1 > minor2) {
            result = 1;
        } else if (minor1 < minor2) {
            result = -1;
        } else {
            if (patch1 > patch2) {
                result = 1;
            } else if (patch1 < patch2) {
                result = -1;
            } else {
                result = 0;
            }
        }
    }

    return result;
}

bool wm_agent_upgrade_validate_task_status_message(const cJSON *input_json, char **status, int *agent_id) {
    if (input_json) {
        cJSON *error_object = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_ERROR]);
        cJSON *data_object = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_ERROR_MESSAGE]);
        cJSON *status_object = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_STATUS]);
        cJSON *agent_json = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_AGENT_ID]);

        if (error_object && (error_object->type == cJSON_Number) && data_object && (data_object->type == cJSON_String) && agent_json
            && (agent_json->type == cJSON_Number)) {

            if (agent_id) {
                *agent_id = agent_json->valueint;
            }

            if (error_object->valueint == WM_UPGRADE_SUCCESS) {
                if (status && status_object && status_object->type == cJSON_String) {
                    os_strdup(status_object->valuestring, *status);
                }
                return true;
            } else {
                mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_TASK_UPDATE_ERROR, error_object->valueint, data_object->valuestring);
            }
        } else {
            mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_REQUIRED_PARAMETERS);
        }
    }
    return false;
}

bool wm_agent_upgrade_validate_task_ids_message(const cJSON *input_json, int *agent_id, int *task_id, char** data) {
    if (input_json) {
        cJSON *agent_json = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_AGENT_ID]);
        cJSON *data_json = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_ERROR_MESSAGE]);
        cJSON *task_json = cJSON_GetObjectItem(input_json, task_manager_json_keys[WM_TASK_TASK_ID]);

        if (agent_id && agent_json && (agent_json->type == cJSON_Number)) {
            *agent_id = agent_json->valueint;
        } else {
            return false;
        }

        if (data && data_json && (data_json->type == cJSON_String)) {
            os_strdup(data_json->valuestring, *data);
        } else {
            return false;
        }

        if (task_id && task_json && (task_json->type == cJSON_Number)) {
            *task_id = task_json->valueint;
        }
        return true;
    }
    return false;
}
