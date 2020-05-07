#pragma once
#include <LinearMath/btIDebugDraw.h>

class GLDebugDrawer : public btIDebugDraw {
private:
	int m_debugMode;
	vector<float> vertices;
public:
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
		vertices.push_back(from.x());
		vertices.push_back(from.y());
		vertices.push_back(from.z());

		vertices.push_back(to.x());
		vertices.push_back(to.y());
		vertices.push_back(to.z());
	}

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {

	}

	virtual void reportErrorWarning(const char* warningString) {

	}

	virtual void draw3dText(const btVector3& location, const char* textString) {

	}

	virtual void setDebugMode(int debugMode) {
		m_debugMode = debugMode;
		vertices.clear();
	}

	virtual int getDebugMode() const { 
		return m_debugMode; 
	}

	virtual void renderLines() {
		unsigned int debugVBO, debugVAO;

		glGenVertexArrays(1, &debugVAO);
		glGenBuffers(1, &debugVBO);

		glBindVertexArray(debugVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDrawArrays(GL_LINES, 0, vertices.size());

		glBindVertexArray(0);
		glDeleteBuffers(1, &debugVBO);
		glDeleteVertexArrays(1, &debugVAO);

		vertices.clear();
	}
};
