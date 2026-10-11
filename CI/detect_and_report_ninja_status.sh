#!/bin/bash

set -eou pipefail
shopt -s globstar extglob

project="$(printf '%s' "${CI_PROJECT_PATH:-OpenMW/openmw}" | jq -sRr '@uri')"

if [[ -z ${GITLAB_ACCESS_TOKEN+xxxxxxx} ]]; then
    echo "This script requires GitLab API access beyond that which CI_JOB_TOKEN grants."
    echo "It must be available via an environment variable called GITLAB_ACCESS_TOKEN."
    echo
    echo "Please create a CI secret with that name containing a Personal Access Token granting the right to call the following endpoints:"
    echo "* /projects/${project}/repository/commits/:sha/statuses"
    echo "* /projects/${project}/pipelines/:pipeline_id"
    echo "* /projects/${project}/repository/commits/:sha"
    echo "* /projects/${project}/pipelines/:pipeline_id/jobs"
    echo "* /projects/${project}/jobs/:job_id/play"
    echo
    echo "At time of writing, this requires Commit Read, Pipeline Read and Job Run access."
    echo
    echo "Alternatively, if running this script locally, create such a token and just export it in your shell."
    exit 1
fi

access_token="${GITLAB_ACCESS_TOKEN:-${CI_JOB_TOKEN}}"
api="${CI_API_V4_URL:-https://gitlab.com/api/v4}"
project_url="$api/projects/$project"

if curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/pipelines/$CI_PIPELINE_ID/jobs?per_page=100" | jq -e '[ .[] | select(.name == "Windows_Ninja_RelWithDebInfo_GroupOne" or .name == "Windows_Ninja_RelWithDebInfo_GroupTwo") | .status == "success" ] | all' > /dev/null; then
    echo "Ninja jobs all passed."
    DESIRED_EXIT_CODE=0
else
    echo "One or more Ninja jobs failed."
    DESIRED_EXIT_CODE=1
fi

echo "Running ReportStatus job with exit code $DESIRED_EXIT_CODE..."
while IFS= read -r job_id; do
    curl --no-progress-meter --request POST --header "PRIVATE-TOKEN: $access_token" --url "$project_url/jobs/$job_id/play" --header "Content-Type: application/json" --data "{ \"job_variables_attributes\": [ { \"key\": \"DESIRED_EXIT_CODE\", \"value\": \"$DESIRED_EXIT_CODE\" } ] }"
done < <(curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/pipelines/$CI_PIPELINE_ID/jobs?per_page=100" | jq '.[] | select(.name == "ReportStatus") | .id')
echo "Done."
