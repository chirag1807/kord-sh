#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 100
#define MAX_CMD_LEN 1024

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct {
    int job_id;           // Job number (1, 2, 3...)
    pid_t pid;            // Process ID
    JobStatus status;     // Running, Stopped, or Done
    char command[MAX_CMD_LEN];  // Command string for display
} Job;

/**
 * Initialize the job control system
 */
void init_jobs(void);

/**
 * Cleanup the job control system
 */
void cleanup_jobs(void);

/**
 * Add a job to the job list
 * Returns job ID on success, -1 on failure
 */
int add_job(pid_t pid, const char *command);

/**
 * Remove a job from the job list by job ID
 */
int remove_job(int job_id);

/**
 * Get a job by job ID
 * Returns pointer to Job or NULL if not found
 */
Job* get_job_by_id(int job_id);

/**
 * Get a job by PID
 * Returns pointer to Job or NULL if not found
 */
Job* get_job_by_pid(pid_t pid);

/**
 * Update job status
 */
void update_job_status(int job_id, JobStatus status);

/**
 * Print all jobs
 */
void print_jobs(void);

/**
 * Check for completed background jobs and clean them up
 * Should be called regularly (e.g., before each prompt)
 */
void check_jobs(void);

/**
 * Bring a background job to foreground
 * Returns 0 on success, -1 on failure
 */
int foreground_job(int job_id);

/**
 * Continue a stopped job in background
 * Returns 0 on success, -1 on failure
 */
int background_job(int job_id);

#endif // JOBS_H

