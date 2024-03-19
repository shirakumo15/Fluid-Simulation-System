#include "Container.h"

namespace Glb {

	GLfloat vertices[] = {
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,

		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
	};

	void Container::resetSize(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void Container::draw()
	{
		glBindVertexArray(VAO);
		
		glLineWidth(2.0f);
		shader->use();
		shader->setMat4("view", Glb::Camera::getInstance().GetView());
		shader->setMat4("projection", Glb::Camera::getInstance().GetProjection());

		glDrawArrays(GL_LINE_LOOP, 0, 4); // 画出底面
		glDrawArrays(GL_LINE_LOOP, 4, 4); // 画出顶面

		for (int i = 8; i < 16; i += 2) {
			glDrawArrays(GL_LINES, i, 2); // 画出每个侧面的四条边
		}


		glBindVertexArray(0);
	}

	void Container::init() {
		std::string vertShaderPath = shaderPath + "/Container.vert";
		std::string fragShaderPath = shaderPath + "/Container.frag";
		shader = new Shader();
		shader->buildFromFile(vertShaderPath, fragShaderPath);

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}
}